#include "web_frontend.hpp"
#include "../infrastructure/logger.hpp"
#include "../infrastructure/config_manager.hpp"
#include <format>
#include <filesystem>

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

void WebFrontend::setupRoutes() {
    setupWishlistRoutes();
    setupCollectionRoutes();
    setupActionRoutes();
    setupStaticRoutes();

    // Home page
    CROW_ROUTE(app_, "/")(
        [this]() {
            return renderHomePage();
        }
    );
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

            // Scrape initially if requested
            if (body.has("scrape_now") && body["scrape_now"].b()) {
                // TODO: Implement initial scraping
            }

            int id = repo.add(item);
            if (id > 0) {
                item.id = id;
                return crow::response(201, wishlistItemToJson(item));
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
                return crow::response(200, "Item updated");
            }

            return crow::response(500, "Failed to update item");
        }
    );

    // Delete wishlist item
    CROW_ROUTE(app_, "/api/wishlist/<int>")
        .methods("DELETE"_method)(
        [](int id) {
            SqliteWishlistRepository repo;
            if (repo.remove(id)) {
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
            item.notes = body.has("notes") ? body["notes"].s() : "";
            item.purchased_at = std::chrono::system_clock::now();
            item.added_at = std::chrono::system_clock::now();

            int id = repo.add(item);
            if (id > 0) {
                item.id = id;
                auto json = collectionItemToJson(item);
                return crow::response(201, json);
            }

            return crow::response(500, "Failed to add item");
        }
    );

    // Delete collection item
    CROW_ROUTE(app_, "/api/collection/<int>")
        .methods("DELETE"_method)(
        [](int id) {
            SqliteCollectionRepository repo;
            if (repo.remove(id)) {
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
                return crow::response(200, response);
            } catch (const std::exception& e) {
                crow::json::wvalue response;
                response["success"] = false;
                response["error"] = e.what();
                return crow::response(500, response);
            }
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
            return res;
        });
}

std::string WebFrontend::renderHomePage() {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Blu-ray Tracker</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial, sans-serif; line-height: 1.6; padding: 20px; background: #f4f4f4; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #333; margin-bottom: 20px; }
        .nav { display: flex; gap: 10px; margin-bottom: 20px; }
        .nav a { padding: 10px 20px; background: #007bff; color: white; text-decoration: none; border-radius: 4px; }
        .nav a:hover { background: #0056b3; }
        .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-bottom: 20px; }
        .stat-card { background: #007bff; color: white; padding: 20px; border-radius: 8px; text-align: center; }
        .stat-card h3 { font-size: 2em; margin-bottom: 10px; }
        .actions { display: flex; gap: 10px; }
        .btn { padding: 10px 20px; background: #28a745; color: white; border: none; border-radius: 4px; cursor: pointer; }
        .btn:hover { background: #218838; }
        .btn-primary { background: #007bff; }
        .btn-primary:hover { background: #0056b3; }
        #message { margin-top: 10px; padding: 10px; border-radius: 4px; display: none; }
        .success { background: #d4edda; color: #155724; }
        .error { background: #f8d7da; color: #721c24; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎬 Blu-ray Tracker</h1>

        <div class="nav">
            <a href="/">Dashboard</a>
            <a href="/wishlist">Wishlist</a>
            <a href="/collection">Collection</a>
        </div>

        <div class="stats" id="stats">
            <div class="stat-card">
                <h3 id="wishlist-count">-</h3>
                <p>Wishlist Items</p>
            </div>
            <div class="stat-card" style="background: #28a745;">
                <h3 id="collection-count">-</h3>
                <p>Collection Items</p>
            </div>
        </div>

        <h2>Actions</h2>
        <div class="actions">
            <button class="btn" onclick="scrapeNow()">Scrape Now</button>
            <button class="btn btn-primary" onclick="window.location.href='/wishlist'">Add to Wishlist</button>
        </div>

        <div id="message"></div>
    </div>

    <script>
        async function loadStats() {
            try {
                const wishlistRes = await fetch('/api/wishlist?size=1');
                const wishlistData = await wishlistRes.json();
                document.getElementById('wishlist-count').textContent = wishlistData.total_count;

                const collectionRes = await fetch('/api/collection?size=1');
                const collectionData = await collectionRes.json();
                document.getElementById('collection-count').textContent = collectionData.total_count;
            } catch (error) {
                console.error('Failed to load stats:', error);
            }
        }

        async function scrapeNow() {
            const btn = event.target;
            btn.disabled = true;
            btn.textContent = 'Scraping...';

            try {
                const res = await fetch('/api/scrape', { method: 'POST' });
                const data = await res.json();

                const msg = document.getElementById('message');
                msg.style.display = 'block';

                if (data.success) {
                    msg.className = 'success';
                    msg.textContent = `Scrape completed! Processed ${data.processed} items.`;
                } else {
                    msg.className = 'error';
                    msg.textContent = `Scrape failed: ${data.error}`;
                }
            } catch (error) {
                const msg = document.getElementById('message');
                msg.style.display = 'block';
                msg.className = 'error';
                msg.textContent = `Error: ${error.message}`;
            } finally {
                btn.disabled = false;
                btn.textContent = 'Scrape Now';
            }
        }

        loadStats();
    </script>
</body>
</html>)HTML";
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
    return json;
}

} // namespace bluray::presentation
