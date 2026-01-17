#include "web_frontend.hpp"
#include "../infrastructure/logger.hpp"
#include "../infrastructure/config_manager.hpp"
#include "../infrastructure/database_manager.hpp"
#include <format>
#include <filesystem>
#include <sstream>
#include <iomanip>

namespace bluray::presentation {

using namespace infrastructure;
using namespace repositories;

WebFrontend::WebFrontend(std::shared_ptr<application::Scheduler> scheduler)
    : scheduler_(std::move(scheduler)) {
    setupRoutes();
}

void WebFrontend::run(int port) {
    Logger::instance().info(std::format("Starting web server on port {}", port));
    app_.port(port).multithreaded().run();
}

void WebFrontend::stop() {
    app_.stop();
    Logger::instance().info("Web server stopped");
}

void WebFrontend::broadcastUpdate(const std::string& message) {
    std::lock_guard<std::mutex> lock(ws_mutex_);
    for (auto* conn : ws_connections_) {
        conn->send_text(message);
    }
}

void WebFrontend::setupRoutes() {
    setupWishlistRoutes();
    setupCollectionRoutes();
    setupReleaseCalendarRoutes();
    setupActionRoutes();
    setupStaticRoutes();
    setupWebSocketRoute();
    setupSettingsRoutes();

    // Home page - SPA
    CROW_ROUTE(app_, "/")(
        [this]() {
            return renderSPA();
        }
    );
}

void WebFrontend::setupWebSocketRoute() {
    CROW_ROUTE(app_, "/ws")
        .websocket()
        .onopen([this](crow::websocket::connection& conn) {
            std::lock_guard<std::mutex> lock(ws_mutex_);
            ws_connections_.insert(&conn);
            Logger::instance().debug(std::format("WebSocket client connected. Total: {}", ws_connections_.size()));
        })
        .onclose([this](crow::websocket::connection& conn, const std::string& /*reason*/) {
            std::lock_guard<std::mutex> lock(ws_mutex_);
            ws_connections_.erase(&conn);
            Logger::instance().debug(std::format("WebSocket client disconnected. Total: {}", ws_connections_.size()));
        })
        .onmessage([](crow::websocket::connection& /*conn*/, const std::string& data, bool /*is_binary*/) {
            Logger::instance().debug(std::format("WebSocket message received: {}", data));
        });
}

void WebFrontend::setupWishlistRoutes() {
    // Get all wishlist items (paginated)
    CROW_ROUTE(app_, "/api/wishlist")
        .methods("GET"_method)(
        [this](const crow::request& req) {
            SqliteWishlistRepository repo;

            int page = 1;
            int page_size = 20;

            if (req.url_params.get("page")) {
                page = std::stoi(req.url_params.get("page"));
            }
            if (req.url_params.get("size")) {
                page_size = std::stoi(req.url_params.get("size"));
            }

            domain::PaginationParams params{page, page_size};
            auto result = repo.findAll(params);

            crow::json::wvalue response;
            response["items"] = crow::json::wvalue::list();
            response["page"] = result.page;
            response["page_size"] = result.page_size;
            response["total_count"] = result.total_count;
            response["total_pages"] = result.total_pages();
            response["has_next"] = result.has_next();
            response["has_previous"] = result.has_previous();

            for (size_t i = 0; i < result.items.size(); ++i) {
                response["items"][i] = wishlistItemToJson(result.items[i]);
            }

            return crow::response(200, response);
        }
    );

    // Add wishlist item
    CROW_ROUTE(app_, "/api/wishlist")
        .methods("POST"_method)(
        [this](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body) {
                return crow::response(400, "Invalid JSON");
            }

            SqliteWishlistRepository repo;

            domain::WishlistItem item;
            item.url = body["url"].s();
            item.title = body["title"].s();
            item.desired_max_price = body["desired_max_price"].d();
            item.notify_on_price_drop = body.has("notify_on_price_drop") ?
                body["notify_on_price_drop"].b() : true;
            item.notify_on_stock = body.has("notify_on_stock") ?
                body["notify_on_stock"].b() : true;
            item.created_at = std::chrono::system_clock::now();
            item.last_checked = std::chrono::system_clock::now();

            // Auto-detect source from URL
            if (item.url.find("amazon.nl") != std::string::npos) {
                item.source = "amazon.nl";
            } else if (item.url.find("bol.com") != std::string::npos) {
                item.source = "bol.com";
            } else {
                item.source = "unknown";
            }

            int id = repo.add(item);
            if (id > 0) {
                item.id = id;

                // Broadcast update via WebSocket
                auto json_item = wishlistItemToJson(item);
                crow::json::wvalue ws_msg;
                ws_msg["type"] = "wishlist_added";
                ws_msg["item"] = std::move(json_item);
                broadcastUpdate(ws_msg.dump());

                json_item = wishlistItemToJson(item);
                return crow::response(201, json_item);
            }

            return crow::response(500, "Failed to add item");
        }
    );

    // Update wishlist item
    CROW_ROUTE(app_, "/api/wishlist/<int>")
        .methods("PUT"_method)(
        [this](const crow::request& req, int id) {
            auto body = crow::json::load(req.body);
            if (!body) {
                return crow::response(400, "Invalid JSON");
            }

            SqliteWishlistRepository repo;
            auto item = repo.findById(id);
            if (!item) {
                return crow::response(404, "Item not found");
            }

            // Update fields
            if (body.has("title")) item->title = body["title"].s();
            if (body.has("desired_max_price")) item->desired_max_price = body["desired_max_price"].d();
            if (body.has("notify_on_price_drop")) item->notify_on_price_drop = body["notify_on_price_drop"].b();
            if (body.has("notify_on_stock")) item->notify_on_stock = body["notify_on_stock"].b();

            if (repo.update(*item)) {
                // Broadcast update via WebSocket
                auto json_item = wishlistItemToJson(*item);
                crow::json::wvalue ws_msg;
                ws_msg["type"] = "wishlist_updated";
                ws_msg["item"] = std::move(json_item);
                broadcastUpdate(ws_msg.dump());

                json_item = wishlistItemToJson(*item);
                return crow::response(200, json_item);
            }

            return crow::response(500, "Failed to update item");
        }
    );

    // Delete wishlist item
    CROW_ROUTE(app_, "/api/wishlist/<int>")
        .methods("DELETE"_method)(
        [this](int id) {
            SqliteWishlistRepository repo;
            if (repo.remove(id)) {
                // Broadcast update via WebSocket
                crow::json::wvalue ws_msg;
                ws_msg["type"] = "wishlist_deleted";
                ws_msg["id"] = id;
                broadcastUpdate(ws_msg.dump());

                return crow::response(200, "Item deleted");
            }
            return crow::response(404, "Item not found");
        }
    );
}

void WebFrontend::setupCollectionRoutes() {
    // Get all collection items (paginated)
    CROW_ROUTE(app_, "/api/collection")
        .methods("GET"_method)(
        [this](const crow::request& req) {
            SqliteCollectionRepository repo;

            int page = 1;
            int page_size = 20;

            if (req.url_params.get("page")) {
                page = std::stoi(req.url_params.get("page"));
            }
            if (req.url_params.get("size")) {
                page_size = std::stoi(req.url_params.get("size"));
            }

            domain::PaginationParams params{page, page_size};
            auto result = repo.findAll(params);

            crow::json::wvalue response;
            response["items"] = crow::json::wvalue::list();
            response["page"] = result.page;
            response["page_size"] = result.page_size;
            response["total_count"] = result.total_count;
            response["total_pages"] = result.total_pages();
            response["has_next"] = result.has_next();
            response["has_previous"] = result.has_previous();

            for (size_t i = 0; i < result.items.size(); ++i) {
                response["items"][i] = collectionItemToJson(result.items[i]);
            }

            return crow::response(200, response);
        }
    );

    // Add collection item
    CROW_ROUTE(app_, "/api/collection")
        .methods("POST"_method)(
        [this](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body) {
                return crow::response(400, "Invalid JSON");
            }

            SqliteCollectionRepository repo;

            domain::CollectionItem item;
            item.url = body["url"].s();
            item.title = body["title"].s();
            item.purchase_price = body["purchase_price"].d();
            item.is_uhd_4k = body.has("is_uhd_4k") ? body["is_uhd_4k"].b() : false;
            item.notes = body.has("notes") ? std::string(body["notes"].s()) : std::string("");
            item.purchased_at = std::chrono::system_clock::now();
            item.added_at = std::chrono::system_clock::now();

            // Auto-detect source from URL
            if (item.url.find("amazon.nl") != std::string::npos) {
                item.source = "amazon.nl";
            } else if (item.url.find("bol.com") != std::string::npos) {
                item.source = "bol.com";
            } else {
                item.source = "unknown";
            }

            int id = repo.add(item);
            if (id > 0) {
                item.id = id;

                // Broadcast update via WebSocket
                auto json_item = collectionItemToJson(item);
                crow::json::wvalue ws_msg;
                ws_msg["type"] = "collection_added";
                ws_msg["item"] = std::move(json_item);
                broadcastUpdate(ws_msg.dump());

                json_item = collectionItemToJson(item);
                return crow::response(201, json_item);
            }

            return crow::response(500, "Failed to add item");
        }
    );

    // Delete collection item
    CROW_ROUTE(app_, "/api/collection/<int>")
        .methods("DELETE"_method)(
        [this](int id) {
            SqliteCollectionRepository repo;
            if (repo.remove(id)) {
                // Broadcast update via WebSocket
                crow::json::wvalue ws_msg;
                ws_msg["type"] = "collection_deleted";
                ws_msg["id"] = id;
                broadcastUpdate(ws_msg.dump());

                return crow::response(200, "Item deleted");
            }
            return crow::response(404, "Item not found");
        }
    );
}

void WebFrontend::setupReleaseCalendarRoutes() {
    using namespace repositories;

    // Get all release calendar items (paginated)
    CROW_ROUTE(app_, "/api/release-calendar")
        .methods("GET"_method)(
        [this](const crow::request& req) {
            SqliteReleaseCalendarRepository repo;

            int page = 1;
            int page_size = 20;

            if (req.url_params.get("page")) {
                page = std::stoi(req.url_params.get("page"));
            }
            if (req.url_params.get("size")) {
                page_size = std::stoi(req.url_params.get("size"));
            }

            domain::PaginationParams params{page, page_size};
            auto result = repo.findAll(params);

            crow::json::wvalue response;
            response["items"] = crow::json::wvalue::list();
            response["page"] = result.page;
            response["page_size"] = result.page_size;
            response["total_count"] = result.total_count;
            response["total_pages"] = result.total_pages();
            response["has_next"] = result.has_next();
            response["has_previous"] = result.has_previous();

            for (size_t i = 0; i < result.items.size(); ++i) {
                response["items"][i] = releaseCalendarItemToJson(result.items[i]);
            }

            return crow::response(200, response);
        }
    );

    // Get release calendar items by date range
    CROW_ROUTE(app_, "/api/release-calendar/range")
        .methods("GET"_method)(
        [this](const crow::request& req) {
            SqliteReleaseCalendarRepository repo;

            std::string start_date = req.url_params.get("start") ? req.url_params.get("start") : "";
            std::string end_date = req.url_params.get("end") ? req.url_params.get("end") : "";

            if (start_date.empty() || end_date.empty()) {
                return crow::response(400, "Missing start or end date");
            }

            // Parse dates (simple implementation)
            std::tm start_tm = {};
            std::tm end_tm = {};
            std::istringstream start_ss(start_date);
            std::istringstream end_ss(end_date);
            start_ss >> std::get_time(&start_tm, "%Y-%m-%d");
            end_ss >> std::get_time(&end_tm, "%Y-%m-%d");

            auto start_tp = std::chrono::system_clock::from_time_t(std::mktime(&start_tm));
            auto end_tp = std::chrono::system_clock::from_time_t(std::mktime(&end_tm));

            auto items = repo.findByDateRange(start_tp, end_tp);

            crow::json::wvalue response;
            response["items"] = crow::json::wvalue::list();
            response["count"] = items.size();

            for (size_t i = 0; i < items.size(); ++i) {
                response["items"][i] = releaseCalendarItemToJson(items[i]);
            }

            return crow::response(200, response);
        }
    );

    // Add release calendar item manually
    CROW_ROUTE(app_, "/api/release-calendar")
        .methods("POST"_method)(
        [this](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body) {
                return crow::response(400, "Invalid JSON");
            }

            SqliteReleaseCalendarRepository repo;

            domain::ReleaseCalendarItem item;
            item.title = body["title"].s();
            item.format = body.has("format") ? std::string(body["format"].s()) : std::string("Blu-ray");
            item.studio = body.has("studio") ? std::string(body["studio"].s()) : std::string("");
            item.product_url = body.has("product_url") ? std::string(body["product_url"].s()) : std::string("");
            item.image_url = body.has("image_url") ? std::string(body["image_url"].s()) : std::string("");
            item.is_uhd_4k = body.has("is_uhd_4k") ? body["is_uhd_4k"].b() : false;
            item.price = body.has("price") ? body["price"].d() : 0.0;
            item.notes = body.has("notes") ? std::string(body["notes"].s()) : std::string("");

            // Parse release date
            if (body.has("release_date")) {
                std::string date_str = body["release_date"].s();
                std::tm tm = {};
                std::istringstream ss(date_str);
                ss >> std::get_time(&tm, "%Y-%m-%d");
                item.release_date = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            } else {
                item.release_date = std::chrono::system_clock::now();
            }

            auto now = std::chrono::system_clock::now();
            item.is_preorder = item.release_date > now;
            item.created_at = now;
            item.last_updated = now;

            int id = repo.add(item);
            if (id > 0) {
                item.id = id;

                // Broadcast update via WebSocket
                auto json_item = releaseCalendarItemToJson(item);
                crow::json::wvalue ws_msg;
                ws_msg["type"] = "calendar_added";
                ws_msg["item"] = std::move(json_item);
                broadcastUpdate(ws_msg.dump());

                json_item = releaseCalendarItemToJson(item);
                return crow::response(201, json_item);
            }

            return crow::response(500, "Failed to add item");
        }
    );

    // Delete release calendar item
    CROW_ROUTE(app_, "/api/release-calendar/<int>")
        .methods("DELETE"_method)(
        [this](int id) {
            SqliteReleaseCalendarRepository repo;
            if (repo.remove(id)) {
                // Broadcast update via WebSocket
                crow::json::wvalue ws_msg;
                ws_msg["type"] = "calendar_deleted";
                ws_msg["id"] = id;
                broadcastUpdate(ws_msg.dump());

                return crow::response(200, "Item deleted");
            }
            return crow::response(404, "Item not found");
        }
    );
}

void WebFrontend::setupActionRoutes() {
    // Trigger scrape now
    CROW_ROUTE(app_, "/api/scrape")
        .methods("POST"_method)(
        [this]() {
            Logger::instance().info("Manual scrape triggered via API");

            try {
                int processed = scheduler_->runOnce();

                crow::json::wvalue response;
                response["success"] = true;
                response["processed"] = processed;

                // Broadcast scrape completion
                crow::json::wvalue ws_msg;
                ws_msg["type"] = "scrape_completed";
                ws_msg["processed"] = processed;
                broadcastUpdate(ws_msg.dump());

                return crow::response(200, response);
            } catch (const std::exception& e) {
                crow::json::wvalue response;
                response["success"] = false;
                response["error"] = e.what();
                return crow::response(500, response);
            }
        }
    );

    // Trigger release calendar scrape
    CROW_ROUTE(app_, "/api/scrape-calendar")
        .methods("POST"_method)(
        [this]() {
            Logger::instance().info("Manual calendar scrape triggered via API");

            try {
                int releases_found = scheduler_->scrapeReleaseCalendar();

                crow::json::wvalue response;
                response["success"] = true;
                response["releases_found"] = releases_found;

                // Broadcast calendar scrape completion
                crow::json::wvalue ws_msg;
                ws_msg["type"] = "calendar_scrape_completed";
                ws_msg["releases_found"] = releases_found;
                broadcastUpdate(ws_msg.dump());

                return crow::response(200, response);
            } catch (const std::exception& e) {
                crow::json::wvalue response;
                response["success"] = false;
                response["error"] = e.what();
                return crow::response(500, response);
            }
        }
    );

    // Get dashboard stats
    CROW_ROUTE(app_, "/api/stats")
        .methods("GET"_method)(
        []() {
            SqliteWishlistRepository wishlist_repo;
            SqliteCollectionRepository collection_repo;

            crow::json::wvalue response;
            response["wishlist_count"] = wishlist_repo.count();
            response["collection_count"] = collection_repo.count();

            // Count in-stock items
            auto wishlist = wishlist_repo.findAll();
            int in_stock_count = 0;
            int uhd_4k_count = 0;
            for (const auto& item : wishlist) {
                if (item.in_stock) in_stock_count++;
                if (item.is_uhd_4k) uhd_4k_count++;
            }
            response["in_stock_count"] = in_stock_count;
            response["uhd_4k_count"] = uhd_4k_count;

            return crow::response(200, response);
        }
    );
}

void WebFrontend::setupSettingsRoutes() {
    // Get settings
    CROW_ROUTE(app_, "/api/settings")
        .methods("GET"_method)(
        []() {
            auto& config = ConfigManager::instance();

            crow::json::wvalue response;
            response["scrape_delay_seconds"] = config.getInt("scrape_delay_seconds", 8);
            response["discord_webhook_url"] = config.get("discord_webhook_url", "");
            response["smtp_server"] = config.get("smtp_server", "");
            response["smtp_port"] = config.get("smtp_port", "587");
            response["smtp_user"] = config.get("smtp_user", "");
            response["smtp_from"] = config.get("smtp_from", "");
            response["smtp_to"] = config.get("smtp_to", "");
            response["web_port"] = config.get("web_port", "8080");
            response["cache_directory"] = config.get("cache_directory", "./cache");

            return crow::response(200, response);
        }
    );

    // Update settings
    CROW_ROUTE(app_, "/api/settings")
        .methods("PUT"_method)(
        [](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body) {
                return crow::response(400, "Invalid JSON");
            }

            auto& config = ConfigManager::instance();

            if (body.has("scrape_delay_seconds")) {
                config.set("scrape_delay_seconds", std::to_string(body["scrape_delay_seconds"].i()));
            }
            if (body.has("discord_webhook_url")) {
                config.set("discord_webhook_url", std::string(body["discord_webhook_url"].s()));
            }
            if (body.has("smtp_server")) {
                config.set("smtp_server", std::string(body["smtp_server"].s()));
            }
            if (body.has("smtp_port")) {
                int smtp_port = body["smtp_port"].i();
                if (smtp_port < 1 || smtp_port > 65535) {
                    return crow::response(400, "Invalid SMTP port");
                }
                config.set("smtp_port", std::to_string(smtp_port));
            }
            if (body.has("smtp_user")) {
                config.set("smtp_user", std::string(body["smtp_user"].s()));
            }
            if (body.has("smtp_pass")) {
                config.set("smtp_pass", std::string(body["smtp_pass"].s()));
            }
            if (body.has("smtp_from")) {
                config.set("smtp_from", std::string(body["smtp_from"].s()));
            }
            if (body.has("smtp_to")) {
                config.set("smtp_to", std::string(body["smtp_to"].s()));
            }

            return crow::response(200, "Settings updated");
        }
    );
}

void WebFrontend::setupStaticRoutes() {
    // Serve cached images
    CROW_ROUTE(app_, "/cache/<string>")
        ([](const std::string& filename) {
            auto& config = ConfigManager::instance();
            const std::string cache_dir = config.get("cache_directory", "./cache");
            const std::filesystem::path file_path = std::filesystem::path(cache_dir) / filename;

            if (!std::filesystem::exists(file_path)) {
                return crow::response(404, "Image not found");
            }

            // Read file
            std::ifstream file(file_path, std::ios::binary);
            if (!file) {
                return crow::response(500, "Failed to read image");
            }

            std::string content((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());

            // Determine content type
            std::string content_type = "image/jpeg";
            if (filename.ends_with(".png")) content_type = "image/png";
            else if (filename.ends_with(".gif")) content_type = "image/gif";
            else if (filename.ends_with(".webp")) content_type = "image/webp";

            crow::response res(content);
            res.set_header("Content-Type", content_type);
            res.set_header("Cache-Control", "public, max-age=31536000");
            return res;
        });
}

std::string WebFrontend::timePointToString(const std::chrono::system_clock::time_point& tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

crow::json::wvalue WebFrontend::wishlistItemToJson(const domain::WishlistItem& item) {
    crow::json::wvalue json;
    json["id"] = item.id;
    json["url"] = item.url;
    json["title"] = item.title;
    json["current_price"] = item.current_price;
    json["desired_max_price"] = item.desired_max_price;
    json["in_stock"] = item.in_stock;
    json["is_uhd_4k"] = item.is_uhd_4k;
    json["image_url"] = item.image_url;
    json["local_image_path"] = item.local_image_path;
    json["source"] = item.source;
    json["notify_on_price_drop"] = item.notify_on_price_drop;
    json["notify_on_stock"] = item.notify_on_stock;
    json["created_at"] = timePointToString(item.created_at);
    json["last_checked"] = timePointToString(item.last_checked);
    return json;
}

crow::json::wvalue WebFrontend::collectionItemToJson(const domain::CollectionItem& item) {
    crow::json::wvalue json;
    json["id"] = item.id;
    json["url"] = item.url;
    json["title"] = item.title;
    json["purchase_price"] = item.purchase_price;
    json["is_uhd_4k"] = item.is_uhd_4k;
    json["image_url"] = item.image_url;
    json["local_image_path"] = item.local_image_path;
    json["source"] = item.source;
    json["notes"] = item.notes;
    json["purchased_at"] = timePointToString(item.purchased_at);
    json["added_at"] = timePointToString(item.added_at);
    return json;
}

crow::json::wvalue WebFrontend::releaseCalendarItemToJson(const domain::ReleaseCalendarItem& item) {
    crow::json::wvalue json;
    json["id"] = item.id;
    json["title"] = item.title;
    json["release_date"] = timePointToString(item.release_date);
    json["format"] = item.format;
    json["studio"] = item.studio;
    json["image_url"] = item.image_url;
    json["local_image_path"] = item.local_image_path;
    json["product_url"] = item.product_url;
    json["is_uhd_4k"] = item.is_uhd_4k;
    json["is_preorder"] = item.is_preorder;
    json["price"] = item.price;
    json["notes"] = item.notes;
    json["created_at"] = timePointToString(item.created_at);
    json["last_updated"] = timePointToString(item.last_updated);
    return json;
}

std::string WebFrontend::renderSPA() {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Blu-ray Tracker - Modern UI</title>
    <style>
        :root {
            --primary: #667eea;
            --primary-dark: #5568d3;
            --secondary: #764ba2;
            --success: #10b981;
            --danger: #ef4444;
            --warning: #f59e0b;
            --info: #3b82f6;
            --bg-primary: #0f172a;
            --bg-secondary: #1e293b;
            --bg-tertiary: #334155;
            --text-primary: #f1f5f9;
            --text-secondary: #cbd5e1;
            --text-muted: #94a3b8;
            --border: #334155;
            --shadow: rgba(0, 0, 0, 0.3);
            --gradient-primary: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            --gradient-success: linear-gradient(135deg, #10b981 0%, #059669 100%);
            --gradient-danger: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
        }

        [data-theme="light"] {
            --bg-primary: #f8fafc;
            --bg-secondary: #ffffff;
            --bg-tertiary: #f1f5f9;
            --text-primary: #0f172a;
            --text-secondary: #475569;
            --text-muted: #64748b;
            --border: #e2e8f0;
            --shadow: rgba(0, 0, 0, 0.1);
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: var(--bg-primary);
            color: var(--text-primary);
            line-height: 1.6;
            transition: all 0.3s ease;
        }

        /* Sidebar */
        .sidebar {
            position: fixed;
            top: 0;
            left: 0;
            width: 260px;
            height: 100vh;
            background: var(--bg-secondary);
            border-right: 1px solid var(--border);
            padding: 2rem 0;
            overflow-y: auto;
            z-index: 100;
            transition: transform 0.3s ease;
        }

        .sidebar-header {
            padding: 0 1.5rem 2rem;
            border-bottom: 1px solid var(--border);
        }

        .logo {
            font-size: 1.5rem;
            font-weight: 700;
            background: var(--gradient-primary);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        .nav-menu {
            padding: 1.5rem 0;
        }

        .nav-item {
            display: flex;
            align-items: center;
            gap: 0.75rem;
            padding: 0.875rem 1.5rem;
            color: var(--text-secondary);
            text-decoration: none;
            transition: all 0.2s ease;
            cursor: pointer;
            border-left: 3px solid transparent;
        }

        .nav-item:hover {
            background: var(--bg-tertiary);
            color: var(--text-primary);
        }

        .nav-item.active {
            background: var(--bg-tertiary);
            color: var(--primary);
            border-left-color: var(--primary);
            font-weight: 600;
        }

        .nav-icon {
            font-size: 1.25rem;
        }

        /* Main Content */
        .main-content {
            margin-left: 260px;
            min-height: 100vh;
            transition: margin-left 0.3s ease;
        }

        .header {
            background: var(--bg-secondary);
            border-bottom: 1px solid var(--border);
            padding: 1.25rem 2rem;
            display: flex;
            justify-content: space-between;
            align-items: center;
            position: sticky;
            top: 0;
            z-index: 90;
        }

        .header-title {
            font-size: 1.75rem;
            font-weight: 700;
        }

        .header-actions {
            display: flex;
            gap: 1rem;
            align-items: center;
        }

        .theme-toggle {
            width: 2.5rem;
            height: 2.5rem;
            border-radius: 50%;
            background: var(--bg-tertiary);
            border: none;
            color: var(--text-primary);
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 1.25rem;
            transition: all 0.2s ease;
        }

        .theme-toggle:hover {
            background: var(--primary);
            color: white;
            transform: rotate(20deg);
        }

        .container {
            padding: 2rem;
            max-width: 1600px;
        }

        /* Cards */
        .card {
            background: var(--bg-secondary);
            border-radius: 1rem;
            padding: 1.5rem;
            border: 1px solid var(--border);
            box-shadow: 0 4px 6px var(--shadow);
            transition: all 0.3s ease;
        }

        .card:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 12px var(--shadow);
        }

        .card-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1.5rem;
        }

        .card-title {
            font-size: 1.25rem;
            font-weight: 600;
        }

        /* Stats Grid */
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 1.5rem;
            margin-bottom: 2rem;
        }

        .stat-card {
            background: var(--gradient-primary);
            border-radius: 1rem;
            padding: 1.5rem;
            color: white;
            box-shadow: 0 4px 6px var(--shadow);
            transition: transform 0.3s ease;
        }

        .stat-card:hover {
            transform: translateY(-4px);
        }

        .stat-card.success {
            background: var(--gradient-success);
        }

        .stat-card.danger {
            background: var(--gradient-danger);
        }

        .stat-icon {
            font-size: 2.5rem;
            opacity: 0.9;
            margin-bottom: 0.5rem;
        }

        .stat-value {
            font-size: 2.5rem;
            font-weight: 700;
            margin-bottom: 0.25rem;
        }

        .stat-label {
            font-size: 0.875rem;
            opacity: 0.9;
        }

        /* Buttons */
        .btn {
            padding: 0.75rem 1.5rem;
            border-radius: 0.5rem;
            border: none;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s ease;
            display: inline-flex;
            align-items: center;
            gap: 0.5rem;
            font-size: 0.875rem;
        }

        .btn-primary {
            background: var(--gradient-primary);
            color: white;
        }

        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 8px var(--shadow);
        }

        .btn-success {
            background: var(--success);
            color: white;
        }

        .btn-danger {
            background: var(--danger);
            color: white;
        }

        .btn-secondary {
            background: var(--bg-tertiary);
            color: var(--text-primary);
        }

        .btn:disabled {
            opacity: 0.6;
            cursor: not-allowed;
            transform: none !important;
        }

        /* Table */
        .table-container {
            overflow-x: auto;
            border-radius: 0.75rem;
            border: 1px solid var(--border);
        }

        table {
            width: 100%;
            border-collapse: collapse;
        }

        th {
            background: var(--bg-tertiary);
            padding: 1rem;
            text-align: left;
            font-weight: 600;
            border-bottom: 1px solid var(--border);
            white-space: nowrap;
        }

        td {
            padding: 1rem;
            border-bottom: 1px solid var(--border);
        }

        tbody tr {
            transition: background 0.2s ease;
        }

        tbody tr:hover {
            background: var(--bg-tertiary);
        }

        .item-image {
            width: 60px;
            height: 90px;
            object-fit: cover;
            border-radius: 0.5rem;
            box-shadow: 0 2px 4px var(--shadow);
        }

        .badge {
            display: inline-block;
            padding: 0.25rem 0.75rem;
            border-radius: 9999px;
            font-size: 0.75rem;
            font-weight: 600;
        }

        .badge-success {
            background: var(--success);
            color: white;
        }

        .badge-danger {
            background: var(--danger);
            color: white;
        }

        .badge-info {
            background: var(--info);
            color: white;
        }

        .badge-warning {
            background: var(--warning);
            color: white;
        }

        /* Modal */
        .modal {
            display: none;
            position: fixed;
            inset: 0;
            z-index: 1000;
            align-items: center;
            justify-content: center;
            background: rgba(0, 0, 0, 0.7);
            backdrop-filter: blur(4px);
            animation: fadeIn 0.2s ease;
        }

        .modal.active {
            display: flex;
        }

        .modal-content {
            background: var(--bg-secondary);
            border-radius: 1rem;
            padding: 2rem;
            max-width: 500px;
            width: 90%;
            max-height: 90vh;
            overflow-y: auto;
            box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.3);
            animation: slideUp 0.3s ease;
        }

        .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1.5rem;
        }

        .modal-title {
            font-size: 1.5rem;
            font-weight: 700;
        }

        .close-modal {
            width: 2rem;
            height: 2rem;
            border-radius: 50%;
            background: var(--bg-tertiary);
            border: none;
            color: var(--text-primary);
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 1.25rem;
        }

        .close-modal:hover {
            background: var(--danger);
            color: white;
        }

        /* Form */
        .form-group {
            margin-bottom: 1.5rem;
        }

        .form-label {
            display: block;
            margin-bottom: 0.5rem;
            font-weight: 600;
            color: var(--text-secondary);
        }

        .form-input {
            width: 100%;
            padding: 0.75rem;
            border: 1px solid var(--border);
            border-radius: 0.5rem;
            background: var(--bg-tertiary);
            color: var(--text-primary);
            font-size: 1rem;
            transition: all 0.2s ease;
        }

        .form-input:focus {
            outline: none;
            border-color: var(--primary);
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }

        .checkbox-group {
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        /* Toast */
        .toast-container {
            position: fixed;
            top: 2rem;
            right: 2rem;
            z-index: 2000;
            display: flex;
            flex-direction: column;
            gap: 1rem;
        }

        .toast {
            min-width: 300px;
            padding: 1rem 1.5rem;
            border-radius: 0.75rem;
            color: white;
            box-shadow: 0 10px 15px -3px var(--shadow);
            display: flex;
            align-items: center;
            gap: 0.75rem;
            animation: slideInRight 0.3s ease;
        }

        .toast-success {
            background: var(--success);
        }

        .toast-error {
            background: var(--danger);
        }

        .toast-info {
            background: var(--info);
        }

        .toast-icon {
            font-size: 1.5rem;
        }

        /* Loading */
        .loading {
            display: inline-block;
            width: 1rem;
            height: 1rem;
            border: 2px solid rgba(255, 255, 255, 0.3);
            border-top-color: white;
            border-radius: 50%;
            animation: spin 0.8s linear infinite;
        }

        /* Search & Filter */
        .search-bar {
            display: flex;
            gap: 1rem;
            margin-bottom: 1.5rem;
            flex-wrap: wrap;
        }

        .search-input {
            flex: 1;
            min-width: 250px;
        }

        .filter-select {
            padding: 0.75rem;
            border: 1px solid var(--border);
            border-radius: 0.5rem;
            background: var(--bg-tertiary);
            color: var(--text-primary);
            cursor: pointer;
        }

        /* Pagination */
        .pagination {
            display: flex;
            justify-content: center;
            gap: 0.5rem;
            margin-top: 2rem;
        }

        .page-btn {
            padding: 0.5rem 1rem;
            border: 1px solid var(--border);
            background: var(--bg-tertiary);
            color: var(--text-primary);
            border-radius: 0.5rem;
            cursor: pointer;
            transition: all 0.2s ease;
        }

        .page-btn:hover:not(:disabled) {
            background: var(--primary);
            color: white;
            border-color: var(--primary);
        }

        .page-btn.active {
            background: var(--primary);
            color: white;
            border-color: var(--primary);
        }

        .page-btn:disabled {
            opacity: 0.4;
            cursor: not-allowed;
        }

        /* Animations */
        @keyframes fadeIn {
            from { opacity: 0; }
            to { opacity: 1; }
        }

        @keyframes slideUp {
            from {
                opacity: 0;
                transform: translateY(20px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        @keyframes slideInRight {
            from {
                opacity: 0;
                transform: translateX(100px);
            }
            to {
                opacity: 1;
                transform: translateX(0);
            }
        }

        @keyframes spin {
            to { transform: rotate(360deg); }
        }

        /* Responsive */
        @media (max-width: 768px) {
            .sidebar {
                transform: translateX(-100%);
            }

            .sidebar.active {
                transform: translateX(0);
            }

            .main-content {
                margin-left: 0;
            }

            .stats-grid {
                grid-template-columns: 1fr;
            }

            .table-container {
                font-size: 0.875rem;
            }

            .container {
                padding: 1rem;
            }
        }

        /* Empty State */
        .empty-state {
            text-align: center;
            padding: 4rem 2rem;
        }

        .empty-icon {
            font-size: 4rem;
            opacity: 0.5;
            margin-bottom: 1rem;
        }

        .empty-title {
            font-size: 1.5rem;
            font-weight: 600;
            margin-bottom: 0.5rem;
        }

        .empty-text {
            color: var(--text-muted);
            margin-bottom: 1.5rem;
        }

        /* Connection Status */
        .connection-status {
            display: flex;
            align-items: center;
            gap: 0.5rem;
            font-size: 0.875rem;
            color: var(--text-muted);
        }

        .connection-dot {
            width: 0.5rem;
            height: 0.5rem;
            border-radius: 50%;
            background: var(--success);
            animation: pulse 2s infinite;
        }

        .connection-dot.disconnected {
            background: var(--danger);
            animation: none;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
    </style>
</head>
<body data-theme="dark">
    <!-- Sidebar -->
    <aside class="sidebar" id="sidebar">
        <div class="sidebar-header">
            <div class="logo">
                <span>🎬</span>
                <span>Blu-ray Tracker</span>
            </div>
        </div>
        <nav class="nav-menu">
            <a class="nav-item active" data-page="dashboard">
                <span class="nav-icon">📊</span>
                <span>Dashboard</span>
            </a>
            <a class="nav-item" data-page="wishlist">
                <span class="nav-icon">⭐</span>
                <span>Wishlist</span>
            </a>
            <a class="nav-item" data-page="collection">
                <span class="nav-icon">📀</span>
                <span>Collection</span>
            </a>
            <a class="nav-item" data-page="settings">
                <span class="nav-icon">⚙️</span>
                <span>Settings</span>
            </a>
        </nav>
    </aside>

    <!-- Main Content -->
    <main class="main-content">
        <header class="header">
            <h1 class="header-title" id="pageTitle">Dashboard</h1>
            <div class="header-actions">
                <div class="connection-status">
                    <span class="connection-dot disconnected" id="wsDot"></span>
                    <span id="wsStatus">Connecting...</span>
                </div>
                <button class="theme-toggle" onclick="toggleTheme()">🌙</button>
            </div>
        </header>

        <div class="container">
            <!-- Dashboard Page -->
            <div id="dashboard-page" class="page-content">
                <div class="stats-grid" id="statsGrid">
                    <div class="stat-card">
                        <div class="stat-icon">⭐</div>
                        <div class="stat-value" id="wishlistCount">0</div>
                        <div class="stat-label">Wishlist Items</div>
                    </div>
                    <div class="stat-card success">
                        <div class="stat-icon">📀</div>
                        <div class="stat-value" id="collectionCount">0</div>
                        <div class="stat-label">Collection Items</div>
                    </div>
                    <div class="stat-card danger">
                        <div class="stat-icon">📦</div>
                        <div class="stat-value" id="inStockCount">0</div>
                        <div class="stat-label">In Stock</div>
                    </div>
                    <div class="stat-card" style="background: linear-gradient(135deg, #f59e0b 0%, #d97706 100%);">
                        <div class="stat-icon">🎞️</div>
                        <div class="stat-value" id="uhd4kCount">0</div>
                        <div class="stat-label">UHD 4K</div>
                    </div>
                </div>

                <div class="card">
                    <div class="card-header">
                        <h2 class="card-title">Quick Actions</h2>
                    </div>
                    <div style="display: flex; gap: 1rem; flex-wrap: wrap;">
                        <button class="btn btn-primary" onclick="openAddWishlistModal()">
                            <span>➕</span>
                            Add to Wishlist
                        </button>
                        <button class="btn btn-success" onclick="openAddCollectionModal()">
                            <span>📀</span>
                            Add to Collection
                        </button>
                        <button class="btn btn-secondary" onclick="scrapeNow()" id="scrapeBtn">
                            <span>🔄</span>
                            Scrape Now
                        </button>
                    </div>
                </div>
            </div>

            <!-- Wishlist Page -->
            <div id="wishlist-page" class="page-content" style="display: none;">
                <div class="card">
                    <div class="card-header">
                        <h2 class="card-title">Wishlist Management</h2>
                        <button class="btn btn-primary" onclick="openAddWishlistModal()">
                            <span>➕</span>
                            Add Item
                        </button>
                    </div>

                    <div class="search-bar">
                        <input type="text" class="form-input search-input" placeholder="Search titles..." id="wishlistSearch">
                        <select class="filter-select" id="wishlistSourceFilter">
                            <option value="">All Sources</option>
                            <option value="amazon.nl">Amazon.nl</option>
                            <option value="bol.com">Bol.com</option>
                        </select>
                        <select class="filter-select" id="wishlistStockFilter">
                            <option value="">All Stock</option>
                            <option value="in_stock">In Stock</option>
                            <option value="out_of_stock">Out of Stock</option>
                        </select>
                    </div>

                    <div class="table-container">
                        <table>
                            <thead>
                                <tr>
                                    <th>Image</th>
                                    <th>Title</th>
                                    <th>Price</th>
                                    <th>Target</th>
                                    <th>Stock</th>
                                    <th>Format</th>
                                    <th>Source</th>
                                    <th>Actions</th>
                                </tr>
                            </thead>
                            <tbody id="wishlistTableBody">
                                <tr>
                                    <td colspan="8" style="text-align: center; padding: 2rem;">
                                        <div class="loading"></div>
                                    </td>
                                </tr>
                            </tbody>
                        </table>
                    </div>

                    <div class="pagination" id="wishlistPagination"></div>
                </div>
            </div>

            <!-- Collection Page -->
            <div id="collection-page" class="page-content" style="display: none;">
                <div class="card">
                    <div class="card-header">
                        <h2 class="card-title">My Collection</h2>
                        <button class="btn btn-success" onclick="openAddCollectionModal()">
                            <span>➕</span>
                            Add Item
                        </button>
                    </div>

                    <div class="search-bar">
                        <input type="text" class="form-input search-input" placeholder="Search titles..." id="collectionSearch">
                        <select class="filter-select" id="collectionSourceFilter">
                            <option value="">All Sources</option>
                            <option value="amazon.nl">Amazon.nl</option>
                            <option value="bol.com">Bol.com</option>
                        </select>
                    </div>

                    <div class="table-container">
                        <table>
                            <thead>
                                <tr>
                                    <th>Image</th>
                                    <th>Title</th>
                                    <th>Price Paid</th>
                                    <th>Format</th>
                                    <th>Source</th>
                                    <th>Notes</th>
                                    <th>Actions</th>
                                </tr>
                            </thead>
                            <tbody id="collectionTableBody">
                                <tr>
                                    <td colspan="7" style="text-align: center; padding: 2rem;">
                                        <div class="loading"></div>
                                    </td>
                                </tr>
                            </tbody>
                        </table>
                    </div>

                    <div class="pagination" id="collectionPagination"></div>
                </div>
            </div>

            <!-- Settings Page -->
            <div id="settings-page" class="page-content" style="display: none;">
                <div class="card">
                    <div class="card-header">
                        <h2 class="card-title">Settings</h2>
                    </div>
                    <form id="settingsForm" onsubmit="saveSettings(event)">
                        <div class="form-group">
                            <label class="form-label">Scrape Delay (seconds)</label>
                            <input type="number" class="form-input" id="scrapeDelay" min="1" max="60">
                        </div>
                        <div class="form-group">
                            <label class="form-label">Discord Webhook URL</label>
                            <input type="url" class="form-input" id="discordWebhook" placeholder="https://discord.com/api/webhooks/...">
                        </div>
                        <div class="form-group">
                            <label class="form-label">SMTP Server</label>
                            <input type="text" class="form-input" id="smtpServer" placeholder="smtp.gmail.com">
                        </div>
                        <div class="form-group">
                            <label class="form-label">SMTP Port</label>
                            <input type="number" class="form-input" id="smtpPort" placeholder="587">
                        </div>
                        <div class="form-group">
                            <label class="form-label">SMTP User</label>
                            <input type="email" class="form-input" id="smtpUser" placeholder="your-email@gmail.com">
                        </div>
                        <div class="form-group">
                            <label class="form-label">SMTP Password</label>
                            <input type="password" class="form-input" id="smtpPass" placeholder="••••••••">
                        </div>
                        <div class="form-group">
                            <label class="form-label">SMTP From</label>
                            <input type="email" class="form-input" id="smtpFrom" placeholder="from@example.com">
                        </div>
                        <div class="form-group">
                            <label class="form-label">SMTP To</label>
                            <input type="email" class="form-input" id="smtpTo" placeholder="to@example.com">
                        </div>
                        <button type="submit" class="btn btn-primary">
                            <span>💾</span>
                            Save Settings
                        </button>
                    </form>
                </div>
            </div>
        </div>
    </main>

    <!-- Add Wishlist Modal -->
    <div class="modal" id="addWishlistModal">
        <div class="modal-content">
            <div class="modal-header">
                <h3 class="modal-title">Add to Wishlist</h3>
                <button class="close-modal" onclick="closeModal('addWishlistModal')">×</button>
            </div>
            <form id="addWishlistForm" onsubmit="addWishlistItem(event)">
                <div class="form-group">
                    <label class="form-label">Product URL</label>
                    <input type="url" class="form-input" id="wishlistUrl" required placeholder="https://www.amazon.nl/...">
                </div>
                <div class="form-group">
                    <label class="form-label">Title</label>
                    <input type="text" class="form-input" id="wishlistTitle" required>
                </div>
                <div class="form-group">
                    <label class="form-label">Desired Max Price (€)</label>
                    <input type="number" class="form-input" id="wishlistPrice" step="0.01" min="0" required>
                </div>
                <div class="form-group">
                    <label class="checkbox-group">
                        <input type="checkbox" id="wishlistNotifyPrice" checked>
                        <span>Notify on price drop</span>
                    </label>
                </div>
                <div class="form-group">
                    <label class="checkbox-group">
                        <input type="checkbox" id="wishlistNotifyStock" checked>
                        <span>Notify when in stock</span>
                    </label>
                </div>
                <button type="submit" class="btn btn-primary" style="width: 100%;">Add to Wishlist</button>
            </form>
        </div>
    </div>

    <!-- Add Collection Modal -->
    <div class="modal" id="addCollectionModal">
        <div class="modal-content">
            <div class="modal-header">
                <h3 class="modal-title">Add to Collection</h3>
                <button class="close-modal" onclick="closeModal('addCollectionModal')">×</button>
            </div>
            <form id="addCollectionForm" onsubmit="addCollectionItem(event)">
                <div class="form-group">
                    <label class="form-label">Product URL</label>
                    <input type="url" class="form-input" id="collectionUrl" required placeholder="https://www.amazon.nl/...">
                </div>
                <div class="form-group">
                    <label class="form-label">Title</label>
                    <input type="text" class="form-input" id="collectionTitle" required>
                </div>
                <div class="form-group">
                    <label class="form-label">Purchase Price (€)</label>
                    <input type="number" class="form-input" id="collectionPrice" step="0.01" min="0" required>
                </div>
                <div class="form-group">
                    <label class="checkbox-group">
                        <input type="checkbox" id="collectionIsUHD">
                        <span>UHD 4K</span>
                    </label>
                </div>
                <div class="form-group">
                    <label class="form-label">Notes (optional)</label>
                    <textarea class="form-input" id="collectionNotes" rows="3"></textarea>
                </div>
                <button type="submit" class="btn btn-success" style="width: 100%;">Add to Collection</button>
            </form>
        </div>
    </div>

    <!-- Edit Wishlist Modal -->
    <div class="modal" id="editWishlistModal">
        <div class="modal-content">
            <div class="modal-header">
                <h3 class="modal-title">Edit Wishlist Item</h3>
                <button class="close-modal" onclick="closeModal('editWishlistModal')">×</button>
            </div>
            <form id="editWishlistForm" onsubmit="updateWishlistItem(event)">
                <input type="hidden" id="editWishlistId">
                <div class="form-group">
                    <label class="form-label">Title</label>
                    <input type="text" class="form-input" id="editWishlistTitle" required>
                </div>
                <div class="form-group">
                    <label class="form-label">Desired Max Price (€)</label>
                    <input type="number" class="form-input" id="editWishlistPrice" step="0.01" min="0" required>
                </div>
                <div class="form-group">
                    <label class="checkbox-group">
                        <input type="checkbox" id="editWishlistNotifyPrice">
                        <span>Notify on price drop</span>
                    </label>
                </div>
                <div class="form-group">
                    <label class="checkbox-group">
                        <input type="checkbox" id="editWishlistNotifyStock">
                        <span>Notify when in stock</span>
                    </label>
                </div>
                <button type="submit" class="btn btn-primary" style="width: 100%;">Update Item</button>
            </form>
        </div>
    </div>

    <!-- Toast Container -->
    <div class="toast-container" id="toastContainer"></div>

    <script>
        // State
        let currentPage = 'dashboard';
        let ws = null;
        let wishlistData = { items: [], page: 1, total_pages: 1 };
        let collectionData = { items: [], page: 1, total_pages: 1 };

        // Initialize
        document.addEventListener('DOMContentLoaded', () => {
            initTheme();
            initNavigation();
            initWebSocket();
            loadDashboardStats();
            loadSettings();
        });

        // Theme
        function initTheme() {
            const theme = localStorage.getItem('theme') || 'dark';
            document.body.setAttribute('data-theme', theme);
            updateThemeIcon();
        }

        function toggleTheme() {
            const current = document.body.getAttribute('data-theme');
            const newTheme = current === 'dark' ? 'light' : 'dark';
            document.body.setAttribute('data-theme', newTheme);
            localStorage.setItem('theme', newTheme);
            updateThemeIcon();
        }

        function updateThemeIcon() {
            const theme = document.body.getAttribute('data-theme');
            document.querySelector('.theme-toggle').textContent = theme === 'dark' ? '☀️' : '🌙';
        }

        // Navigation
        function initNavigation() {
            document.querySelectorAll('.nav-item').forEach(item => {
                item.addEventListener('click', () => {
                    const page = item.getAttribute('data-page');
                    navigateTo(page);
                });
            });
        }

        function navigateTo(page) {
            currentPage = page;

            // Update nav
            document.querySelectorAll('.nav-item').forEach(item => {
                item.classList.toggle('active', item.getAttribute('data-page') === page);
            });

            // Update pages
            document.querySelectorAll('.page-content').forEach(p => p.style.display = 'none');
            document.getElementById(`${page}-page`).style.display = 'block';

            // Update title
            const titles = {
                dashboard: 'Dashboard',
                wishlist: 'Wishlist',
                collection: 'Collection',
                settings: 'Settings'
            };
            document.getElementById('pageTitle').textContent = titles[page];

            // Load data
            if (page === 'wishlist') loadWishlist();
            if (page === 'collection') loadCollection();
            if (page === 'settings') loadSettings();
        }

        // WebSocket
        function initWebSocket() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = `${protocol}//${window.location.host}/ws`;

            ws = new WebSocket(wsUrl);

            ws.onopen = () => {
                updateConnectionStatus(true);
                showToast('Connected to live updates', 'success');
            };

            ws.onclose = () => {
                updateConnectionStatus(false);
                setTimeout(initWebSocket, 5000);
            };

            ws.onerror = () => {
                updateConnectionStatus(false);
            };

            ws.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data);
                    handleWebSocketMessage(data);
                } catch (e) {
                    console.error('WebSocket message error:', e);
                }
            };
        }

        function updateConnectionStatus(connected) {
            const dot = document.getElementById('wsDot');
            const status = document.getElementById('wsStatus');

            if (connected) {
                dot.classList.remove('disconnected');
                status.textContent = 'Live';
            } else {
                dot.classList.add('disconnected');
                status.textContent = 'Disconnected';
            }
        }

        function handleWebSocketMessage(data) {
            switch (data.type) {
                case 'wishlist_added':
                case 'wishlist_updated':
                case 'wishlist_deleted':
                    if (currentPage === 'wishlist') loadWishlist();
                    loadDashboardStats();
                    showToast('Wishlist updated', 'info');
                    break;
                case 'collection_added':
                case 'collection_deleted':
                    if (currentPage === 'collection') loadCollection();
                    loadDashboardStats();
                    showToast('Collection updated', 'info');
                    break;
                case 'scrape_completed':
                    showToast(`Scrape completed! Processed ${data.processed} items`, 'success');
                    if (currentPage === 'wishlist') loadWishlist();
                    loadDashboardStats();
                    break;
            }
        }

        // Dashboard
        async function loadDashboardStats() {
            try {
                const res = await fetch('/api/stats');
                const data = await res.json();

                document.getElementById('wishlistCount').textContent = data.wishlist_count;
                document.getElementById('collectionCount').textContent = data.collection_count;
                document.getElementById('inStockCount').textContent = data.in_stock_count;
                document.getElementById('uhd4kCount').textContent = data.uhd_4k_count;
            } catch (error) {
                console.error('Failed to load stats:', error);
            }
        }

        async function scrapeNow() {
            const btn = document.getElementById('scrapeBtn');
            const originalText = btn.innerHTML;
            btn.disabled = true;
            btn.innerHTML = '<span class="loading"></span> Scraping...';

            try {
                const res = await fetch('/api/scrape', { method: 'POST' });
                const data = await res.json();

                if (data.success) {
                    showToast(`Scrape completed! Processed ${data.processed} items`, 'success');
                    loadDashboardStats();
                } else {
                    showToast(`Scrape failed: ${data.error}`, 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            } finally {
                btn.disabled = false;
                btn.innerHTML = originalText;
            }
        }

        // Wishlist
        async function loadWishlist(page = 1) {
            try {
                const res = await fetch(`/api/wishlist?page=${page}&size=20`);
                wishlistData = await res.json();
                renderWishlistTable();
                renderWishlistPagination();
            } catch (error) {
                console.error('Failed to load wishlist:', error);
                showToast('Failed to load wishlist', 'error');
            }
        }

        function renderWishlistTable() {
            const tbody = document.getElementById('wishlistTableBody');

            if (wishlistData.items.length === 0) {
                tbody.innerHTML = `
                    <tr>
                        <td colspan="8">
                            <div class="empty-state">
                                <div class="empty-icon">⭐</div>
                                <div class="empty-title">No wishlist items yet</div>
                                <div class="empty-text">Add your first item to start tracking prices</div>
                                <button class="btn btn-primary" onclick="openAddWishlistModal()">Add Item</button>
                            </div>
                        </td>
                    </tr>
                `;
                return;
            }

            tbody.innerHTML = wishlistData.items.map(item => `
                <tr>
                    <td>
                        ${item.local_image_path ?
                            `<img src="/cache/${item.local_image_path.split('/').pop()}" class="item-image" alt="${item.title}">` :
                            '<div class="item-image" style="background: var(--bg-tertiary); display: flex; align-items: center; justify-content: center;">📀</div>'
                        }
                    </td>
                    <td>
                        <div style="font-weight: 600; margin-bottom: 0.25rem;">${escapeHtml(item.title)}</div>
                        <a href="${item.url}" target="_blank" style="color: var(--primary); font-size: 0.875rem;">View Product →</a>
                    </td>
                    <td>
                        <div style="font-weight: 600; color: var(--success);">€${item.current_price.toFixed(2)}</div>
                    </td>
                    <td>€${item.desired_max_price.toFixed(2)}</td>
                    <td>
                        ${item.in_stock ?
                            '<span class="badge badge-success">In Stock</span>' :
                            '<span class="badge badge-danger">Out of Stock</span>'
                        }
                    </td>
                    <td>
                        ${item.is_uhd_4k ?
                            '<span class="badge badge-warning">UHD 4K</span>' :
                            '<span class="badge badge-info">Blu-ray</span>'
                        }
                    </td>
                    <td>${item.source}</td>
                    <td>
                        <div style="display: flex; gap: 0.5rem;">
                            <button class="btn btn-secondary" style="padding: 0.5rem 0.75rem;" onclick="editWishlistItem(${item.id})">✏️</button>
                            <button class="btn btn-danger" style="padding: 0.5rem 0.75rem;" onclick="deleteWishlistItem(${item.id})">🗑️</button>
                        </div>
                    </td>
                </tr>
            `).join('');
        }

        function renderWishlistPagination() {
            const container = document.getElementById('wishlistPagination');
            const { page, total_pages, has_previous, has_next } = wishlistData;

            let html = '';
            html += `<button class="page-btn" ${!has_previous ? 'disabled' : ''} onclick="loadWishlist(${page - 1})">← Prev</button>`;

            for (let i = 1; i <= total_pages; i++) {
                if (i === 1 || i === total_pages || (i >= page - 2 && i <= page + 2)) {
                    html += `<button class="page-btn ${i === page ? 'active' : ''}" onclick="loadWishlist(${i})">${i}</button>`;
                } else if (i === page - 3 || i === page + 3) {
                    html += '<span style="padding: 0.5rem;">...</span>';
                }
            }

            html += `<button class="page-btn" ${!has_next ? 'disabled' : ''} onclick="loadWishlist(${page + 1})">Next →</button>`;
            container.innerHTML = html;
        }

        async function addWishlistItem(event) {
            event.preventDefault();

            const data = {
                url: document.getElementById('wishlistUrl').value,
                title: document.getElementById('wishlistTitle').value,
                desired_max_price: parseFloat(document.getElementById('wishlistPrice').value),
                notify_on_price_drop: document.getElementById('wishlistNotifyPrice').checked,
                notify_on_stock: document.getElementById('wishlistNotifyStock').checked
            };

            try {
                const res = await fetch('/api/wishlist', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });

                if (res.ok) {
                    showToast('Item added to wishlist', 'success');
                    closeModal('addWishlistModal');
                    document.getElementById('addWishlistForm').reset();
                    if (currentPage === 'wishlist') loadWishlist();
                    loadDashboardStats();
                } else {
                    showToast('Failed to add item', 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            }
        }

        function editWishlistItem(id) {
            const item = wishlistData.items.find(i => i.id === id);
            if (!item) return;

            document.getElementById('editWishlistId').value = item.id;
            document.getElementById('editWishlistTitle').value = item.title;
            document.getElementById('editWishlistPrice').value = item.desired_max_price;
            document.getElementById('editWishlistNotifyPrice').checked = item.notify_on_price_drop;
            document.getElementById('editWishlistNotifyStock').checked = item.notify_on_stock;

            openModal('editWishlistModal');
        }

        async function updateWishlistItem(event) {
            event.preventDefault();

            const id = document.getElementById('editWishlistId').value;
            const data = {
                title: document.getElementById('editWishlistTitle').value,
                desired_max_price: parseFloat(document.getElementById('editWishlistPrice').value),
                notify_on_price_drop: document.getElementById('editWishlistNotifyPrice').checked,
                notify_on_stock: document.getElementById('editWishlistNotifyStock').checked
            };

            try {
                const res = await fetch(`/api/wishlist/${id}`, {
                    method: 'PUT',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });

                if (res.ok) {
                    showToast('Item updated', 'success');
                    closeModal('editWishlistModal');
                    loadWishlist(wishlistData.page);
                } else {
                    showToast('Failed to update item', 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            }
        }

        async function deleteWishlistItem(id) {
            if (!confirm('Are you sure you want to delete this item?')) return;

            try {
                const res = await fetch(`/api/wishlist/${id}`, { method: 'DELETE' });
                if (res.ok) {
                    showToast('Item deleted', 'success');
                    loadWishlist(wishlistData.page);
                    loadDashboardStats();
                } else {
                    showToast('Failed to delete item', 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            }
        }

        // Collection
        async function loadCollection(page = 1) {
            try {
                const res = await fetch(`/api/collection?page=${page}&size=20`);
                collectionData = await res.json();
                renderCollectionTable();
                renderCollectionPagination();
            } catch (error) {
                console.error('Failed to load collection:', error);
                showToast('Failed to load collection', 'error');
            }
        }

        function renderCollectionTable() {
            const tbody = document.getElementById('collectionTableBody');

            if (collectionData.items.length === 0) {
                tbody.innerHTML = `
                    <tr>
                        <td colspan="7">
                            <div class="empty-state">
                                <div class="empty-icon">📀</div>
                                <div class="empty-title">No collection items yet</div>
                                <div class="empty-text">Add your first item to start building your collection</div>
                                <button class="btn btn-success" onclick="openAddCollectionModal()">Add Item</button>
                            </div>
                        </td>
                    </tr>
                `;
                return;
            }

            tbody.innerHTML = collectionData.items.map(item => `
                <tr>
                    <td>
                        ${item.local_image_path ?
                            `<img src="/cache/${item.local_image_path.split('/').pop()}" class="item-image" alt="${item.title}">` :
                            '<div class="item-image" style="background: var(--bg-tertiary); display: flex; align-items: center; justify-content: center;">📀</div>'
                        }
                    </td>
                    <td>
                        <div style="font-weight: 600; margin-bottom: 0.25rem;">${escapeHtml(item.title)}</div>
                        <a href="${item.url}" target="_blank" style="color: var(--primary); font-size: 0.875rem;">View Product →</a>
                    </td>
                    <td>€${item.purchase_price.toFixed(2)}</td>
                    <td>
                        ${item.is_uhd_4k ?
                            '<span class="badge badge-warning">UHD 4K</span>' :
                            '<span class="badge badge-info">Blu-ray</span>'
                        }
                    </td>
                    <td>${item.source}</td>
                    <td>${item.notes ? escapeHtml(item.notes.substring(0, 50)) + (item.notes.length > 50 ? '...' : '') : '-'}</td>
                    <td>
                        <button class="btn btn-danger" style="padding: 0.5rem 0.75rem;" onclick="deleteCollectionItem(${item.id})">🗑️</button>
                    </td>
                </tr>
            `).join('');
        }

        function renderCollectionPagination() {
            const container = document.getElementById('collectionPagination');
            const { page, total_pages, has_previous, has_next } = collectionData;

            let html = '';
            html += `<button class="page-btn" ${!has_previous ? 'disabled' : ''} onclick="loadCollection(${page - 1})">← Prev</button>`;

            for (let i = 1; i <= total_pages; i++) {
                if (i === 1 || i === total_pages || (i >= page - 2 && i <= page + 2)) {
                    html += `<button class="page-btn ${i === page ? 'active' : ''}" onclick="loadCollection(${i})">${i}</button>`;
                } else if (i === page - 3 || i === page + 3) {
                    html += '<span style="padding: 0.5rem;">...</span>';
                }
            }

            html += `<button class="page-btn" ${!has_next ? 'disabled' : ''} onclick="loadCollection(${page + 1})">Next →</button>`;
            container.innerHTML = html;
        }

        async function addCollectionItem(event) {
            event.preventDefault();

            const data = {
                url: document.getElementById('collectionUrl').value,
                title: document.getElementById('collectionTitle').value,
                purchase_price: parseFloat(document.getElementById('collectionPrice').value),
                is_uhd_4k: document.getElementById('collectionIsUHD').checked,
                notes: document.getElementById('collectionNotes').value
            };

            try {
                const res = await fetch('/api/collection', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });

                if (res.ok) {
                    showToast('Item added to collection', 'success');
                    closeModal('addCollectionModal');
                    document.getElementById('addCollectionForm').reset();
                    if (currentPage === 'collection') loadCollection();
                    loadDashboardStats();
                } else {
                    showToast('Failed to add item', 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            }
        }

        async function deleteCollectionItem(id) {
            if (!confirm('Are you sure you want to delete this item?')) return;

            try {
                const res = await fetch(`/api/collection/${id}`, { method: 'DELETE' });
                if (res.ok) {
                    showToast('Item deleted', 'success');
                    loadCollection(collectionData.page);
                    loadDashboardStats();
                } else {
                    showToast('Failed to delete item', 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            }
        }

        // Settings
        async function loadSettings() {
            try {
                const res = await fetch('/api/settings');
                const data = await res.json();

                document.getElementById('scrapeDelay').value = data.scrape_delay_seconds;
                document.getElementById('discordWebhook').value = data.discord_webhook_url;
                document.getElementById('smtpServer').value = data.smtp_server;
                document.getElementById('smtpPort').value = data.smtp_port;
                document.getElementById('smtpUser').value = data.smtp_user;
                document.getElementById('smtpFrom').value = data.smtp_from;
                document.getElementById('smtpTo').value = data.smtp_to;
            } catch (error) {
                console.error('Failed to load settings:', error);
            }
        }

        async function saveSettings(event) {
            event.preventDefault();

            const data = {
                scrape_delay_seconds: parseInt(document.getElementById('scrapeDelay').value),
                discord_webhook_url: document.getElementById('discordWebhook').value,
                smtp_server: document.getElementById('smtpServer').value,
                smtp_port: document.getElementById('smtpPort').value,
                smtp_user: document.getElementById('smtpUser').value,
                smtp_pass: document.getElementById('smtpPass').value,
                smtp_from: document.getElementById('smtpFrom').value,
                smtp_to: document.getElementById('smtpTo').value
            };

            try {
                const res = await fetch('/api/settings', {
                    method: 'PUT',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });

                if (res.ok) {
                    showToast('Settings saved successfully', 'success');
                    document.getElementById('smtpPass').value = '';
                } else {
                    showToast('Failed to save settings', 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            }
        }

        // Modals
        function openModal(id) {
            document.getElementById(id).classList.add('active');
        }

        function closeModal(id) {
            document.getElementById(id).classList.remove('active');
        }

        function openAddWishlistModal() {
            openModal('addWishlistModal');
        }

        function openAddCollectionModal() {
            openModal('addCollectionModal');
        }

        // Close modals on backdrop click
        document.querySelectorAll('.modal').forEach(modal => {
            modal.addEventListener('click', (e) => {
                if (e.target === modal) {
                    closeModal(modal.id);
                }
            });
        });

        // Toast
        function showToast(message, type = 'info') {
            const container = document.getElementById('toastContainer');
            const toast = document.createElement('div');
            toast.className = `toast toast-${type}`;

            const icons = {
                success: '✓',
                error: '✕',
                info: 'ℹ'
            };

            toast.innerHTML = `
                <span class="toast-icon">${icons[type]}</span>
                <span>${escapeHtml(message)}</span>
            `;

            container.appendChild(toast);

            setTimeout(() => {
                toast.style.opacity = '0';
                setTimeout(() => toast.remove(), 300);
            }, 5000);
        }

        // Utilities
        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }
    </script>
</body>
</html>)HTML";
}

} // namespace bluray::presentation
