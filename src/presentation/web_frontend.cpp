#include "web_frontend.hpp"
#include "../application/scraper/scraper.hpp"
#include "../application/enrichment/tmdb_enrichment_service.hpp"
#include "../infrastructure/config_manager.hpp"
#include "../infrastructure/database_manager.hpp"
#include "../infrastructure/input_validation.hpp"
#include "../infrastructure/logger.hpp"
#include "../infrastructure/repositories/collection_repository.hpp"
#include "../infrastructure/repositories/price_history_repository.hpp"
#include "../infrastructure/repositories/release_calendar_repository.hpp"
#include "../infrastructure/repositories/tag_repository.hpp"
#include "../infrastructure/repositories/wishlist_repository.hpp"
#include "html_renderer.hpp"
#include <filesystem>
#include <fmt/format.h>
#include <iomanip>
#include <sstream>

namespace bluray::presentation {

using namespace infrastructure;
using namespace repositories;

// Helper function to update TMDb/IMDb metadata fields with validation
namespace {
void updateMetadataFields(const crow::json::rvalue &body, 
                         int &tmdb_id, std::string &imdb_id, 
                         double &tmdb_rating, std::string &trailer_key) {
  if (body.has("tmdb_id")) {
    tmdb_id = body["tmdb_id"].i();
  }
  
  if (body.has("imdb_id")) {
    std::string id = body["imdb_id"].s();
    if (validation::isValidImdbId(id)) {
      imdb_id = id;
    } else {
      Logger::instance().warning(fmt::format(
          "Ignoring invalid imdb_id value '{}' (must be tt followed by 7-8 digits)", 
          validation::sanitizeForLog(id)));
    }
  }
  
  if (body.has("tmdb_rating")) {
    double rating = body["tmdb_rating"].d();
    if (validation::isValidTmdbRating(rating)) {
      tmdb_rating = rating;
    } else {
      Logger::instance().warning(fmt::format(
          "Ignoring invalid tmdb_rating value {} (must be between 0.0 and 10.0)", 
          rating));
    }
  }
  
  if (body.has("trailer_key")) {
    std::string key = body["trailer_key"].s();
    if (validation::isValidTrailerKey(key)) {
      trailer_key = key;
    } else {
      Logger::instance().warning(fmt::format(
          "Ignoring invalid trailer_key value '{}' (must be 11 alphanumeric characters with - or _)", 
          validation::sanitizeForLog(key)));
    }
  }
}

// Helper function to update edition & bonus features fields
void updateEditionFields(const crow::json::rvalue &body,
                        std::string &edition_type, bool &has_slipcover,
                        bool &has_digital_copy, std::string &bonus_features) {
  if (body.has("edition_type")) {
    edition_type = body["edition_type"].s();
  }
  if (body.has("has_slipcover")) {
    has_slipcover = body["has_slipcover"].b();
  }
  if (body.has("has_digital_copy")) {
    has_digital_copy = body["has_digital_copy"].b();
  }
  if (body.has("bonus_features")) {
    bonus_features = body["bonus_features"].s();
  }
}

// Helper function to populate tag JSON array
void populateTagJson(crow::json::wvalue &json, int item_id, 
                     const std::string &item_type) {
  SqliteTagRepository tag_repo;
  auto tags = tag_repo.getTagsForItem(item_id, item_type);
  json["tags"] = crow::json::wvalue::list();
  for (size_t i = 0; i < tags.size(); ++i) {
    crow::json::wvalue tag_json;
    tag_json["id"] = tags[i].id;
    tag_json["name"] = tags[i].name;
    tag_json["color"] = tags[i].color;
    json["tags"][i] = std::move(tag_json);
  }
}
} // anonymous namespace

WebFrontend::WebFrontend(std::shared_ptr<application::Scheduler> scheduler)
    : scheduler_(std::move(scheduler)),
      renderer_(std::make_unique<HtmlRenderer>()) {
  setupRoutes();
}

void WebFrontend::run(int port) {
  Logger::instance().info(fmt::format("Starting web server on port {}", port));
  app_.port(port).multithreaded().run();
}

void WebFrontend::stop() {
  app_.stop();
  Logger::instance().info("Web server stopped");
}

void WebFrontend::broadcastUpdate(const std::string &message) {
  std::lock_guard<std::mutex> lock(ws_mutex_);
  for (auto *conn : ws_connections_) {
    conn->send_text(message);
  }
}

void WebFrontend::setupRoutes() {
  setupWishlistRoutes();
  setupCollectionRoutes();
  setupReleaseCalendarRoutes();
  setupTagRoutes();
  setupActionRoutes();
  setupEnrichmentRoutes();
  setupStaticRoutes();
  setupWebSocketRoute();
  setupSettingsRoutes();

  // Home page - SPA
  CROW_ROUTE(app_, "/")([this]() { return renderSPA(); });
}

void WebFrontend::setupWebSocketRoute() {
  CROW_ROUTE(app_, "/ws")
      .websocket(&app_)
      .onopen([this](crow::websocket::connection &conn) {
        std::lock_guard<std::mutex> lock(ws_mutex_);
        ws_connections_.insert(&conn);
        Logger::instance().debug(fmt::format(
            "WebSocket client connected. Total: {}", ws_connections_.size()));
      })
      .onclose([this](crow::websocket::connection &conn,
                      const std::string & /*reason*/) {
        std::lock_guard<std::mutex> lock(ws_mutex_);
        ws_connections_.erase(&conn);
        Logger::instance().debug(
            fmt::format("WebSocket client disconnected. Total: {}",
                        ws_connections_.size()));
      })
      .onmessage([](crow::websocket::connection & /*conn*/,
                    const std::string &data, bool /*is_binary*/) {
        Logger::instance().debug(
            fmt::format("WebSocket message received: {}", data));
      });
}

void WebFrontend::setupWishlistRoutes() {
  // Get all wishlist items (paginated)
  CROW_ROUTE(app_, "/api/wishlist")
      .methods("GET"_method)([this](const crow::request &req) {
        SqliteWishlistRepository repo;

        int page = 1;
        int page_size = 20;

        if (req.url_params.get("page")) {
          page = std::stoi(req.url_params.get("page"));
        }
        if (req.url_params.get("size")) {
          page_size = std::stoi(req.url_params.get("size"));
        }

        domain::PaginationParams params;
        params.page = page;
        params.page_size = page_size;

        if (req.url_params.get("sort")) {
          params.sort_by = req.url_params.get("sort");
        }
        if (req.url_params.get("order")) {
          params.sort_order = req.url_params.get("order");
        }
        if (req.url_params.get("stock")) {
          params.filter_stock = req.url_params.get("stock");
        }
        if (req.url_params.get("source")) {
          params.filter_source = req.url_params.get("source");
        }
        if (req.url_params.get("search")) {
          params.search_query = req.url_params.get("search");
        }

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
      });

  // Add wishlist item
  CROW_ROUTE(app_, "/api/wishlist")
      .methods("POST"_method)([this](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body) {
          return crow::response(400, "Invalid JSON");
        }

        SqliteWishlistRepository repo;

        domain::WishlistItem item;
        item.url = body["url"].s();
        item.title = body.has("title") ? body["title"].s() : std::string("");
        item.desired_max_price = body["desired_max_price"].d();
        item.notify_on_price_drop = body.has("notify_on_price_drop")
                                        ? body["notify_on_price_drop"].b()
                                        : true;
        item.notify_on_stock =
            body.has("notify_on_stock") ? body["notify_on_stock"].b() : true;
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

        // Try to scrape metadata if available
        if (auto sc = application::scraper::ScraperFactory::create(item.url)) {
          if (auto product = sc->scrape(item.url)) {
            if (item.title.empty() && !product->title.empty()) {
              item.title = product->title;
            }
            if (item.image_url.empty() && !product->image_url.empty()) {
              item.image_url = product->image_url;
            }
            // If user didn't specify UHD status, take from scraper
            if (!body.has("is_uhd_4k")) {
              item.is_uhd_4k = product->is_uhd_4k;
            }
            // Set initial stock status from scrape
            item.in_stock = product->in_stock;
            item.current_price = product->price;
          }
        }

        int id = repo.add(item);
        if (id > 0) {
          item.id = id;

          // Record initial price history
          infrastructure::PriceHistoryRepository history_repo;
          history_repo.addEntry(item.id, item.current_price, item.in_stock);

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
      });

  // Update wishlist item
  CROW_ROUTE(app_, "/api/wishlist/<int>")
      .methods("PUT"_method)([this](const crow::request &req, int id) {
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
        bool title_changed = false;
        if (body.has("title")) {
          std::string new_title = body["title"].s();
          if (new_title != item->title) {
            item->title = new_title;
            // Lock title if manually changed
            item->title_locked = true;
            title_changed = true;
          }
        }
        if (body.has("title_locked")) {
          // Only apply explicit lock setting if title didn't change (user just
          // toggling lock) or if user explicitly requested a lock state
          // (overriding our auto-lock) Actually, if title changed, we WANT to
          // lock it. If user also sent "false" (unchecked box), we should
          // ignore that "false" because it's just the old state. The only time
          // we respect "false" here is if title didn't change.
          bool explicit_lock = body["title_locked"].b();
          if (!title_changed || explicit_lock) {
            item->title_locked = explicit_lock;
          }
        }
        if (body.has("desired_max_price"))
          item->desired_max_price = body["desired_max_price"].d();
        if (body.has("notify_on_price_drop"))
          item->notify_on_price_drop = body["notify_on_price_drop"].b();
        if (body.has("notify_on_stock"))
          item->notify_on_stock = body["notify_on_stock"].b();

        // TMDb/IMDb integration fields with validation
        updateMetadataFields(body, item->tmdb_id, item->imdb_id, 
                           item->tmdb_rating, item->trailer_key);

        // Edition & bonus features fields
        updateEditionFields(body, item->edition_type, item->has_slipcover,
                          item->has_digital_copy, item->bonus_features);

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
      });

  // Delete wishlist item
  CROW_ROUTE(app_, "/api/wishlist/<int>")
      .methods("DELETE"_method)([this](int id) {
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
      });
  // Get price history for wishlist item
  CROW_ROUTE(app_, "/api/wishlist/<int>/history")
      .methods("GET"_method)([](int id) {
        SqliteWishlistRepository wishlist_repo;
        auto item = wishlist_repo.findById(id);
        if (!item) {
          return crow::response(404, "Item not found");
        }

        infrastructure::PriceHistoryRepository repo;
        int days = 180; // Default to 6 months

        // Allow days override via query param
        // Note: Crow's url_params handling might need checking, assuming
        // minimal here or default. Using default 180 days for now or
        // hardcoded as per plan.

        auto history = repo.getHistory(id, days);

        crow::json::wvalue response;
        response = crow::json::wvalue::list();

        for (size_t i = 0; i < history.size(); ++i) {
          crow::json::wvalue entry;
          entry["price"] = history[i].price;
          entry["in_stock"] = history[i].in_stock;
          entry["date"] = history[i].recorded_at;
          response[i] = std::move(entry);
        }

        return crow::response(200, response);
      });
}

void WebFrontend::setupCollectionRoutes() {
  // Get all collection items (paginated)
  CROW_ROUTE(app_, "/api/collection")
      .methods("GET"_method)([this](const crow::request &req) {
        SqliteCollectionRepository repo;

        int page = 1;
        int page_size = 20;

        if (req.url_params.get("page")) {
          page = std::stoi(req.url_params.get("page"));
        }
        if (req.url_params.get("size")) {
          page_size = std::stoi(req.url_params.get("size"));
        }

        domain::PaginationParams params;
        params.page = page;
        params.page_size = page_size;

        if (req.url_params.get("source")) {
          params.filter_source = req.url_params.get("source");
        }
        if (req.url_params.get("search")) {
          params.search_query = req.url_params.get("search");
        }
        // Collection currently only supports source filter, but we could add
        // sort later

        auto result = repo.findAll(params);

        crow::json::wvalue response;
        response["items"] = crow::json::wvalue::list();
        response["page"] = result.page;
        response["page_size"] = result.page_size;
        response["total_count"] = result.total_count;
        response["total_value"] = result.total_value;
        response["total_pages"] = result.total_pages();
        response["has_next"] = result.has_next();
        response["has_previous"] = result.has_previous();

        for (size_t i = 0; i < result.items.size(); ++i) {
          response["items"][i] = collectionItemToJson(result.items[i]);
        }

        return crow::response(200, response);
      });

  // Add collection item
  CROW_ROUTE(app_, "/api/collection")
      .methods("POST"_method)([this](const crow::request &req) {
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
        item.notes = body.has("notes") ? std::string(body["notes"].s())
                                       : std::string("");
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

        // Try to scrape metadata if available
        if (auto sc = application::scraper::ScraperFactory::create(item.url)) {
          if (auto product = sc->scrape(item.url)) {
            if (item.title.empty() && !product->title.empty()) {
              item.title = product->title;
            }
            if (item.image_url.empty() && !product->image_url.empty()) {
              item.image_url = product->image_url;
            }
            // If user didn't specify UHD status, take from scraper
            if (!body.has("is_uhd_4k")) {
              item.is_uhd_4k = product->is_uhd_4k;
            }
          }
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
      });

  // Update collection item
  CROW_ROUTE(app_, "/api/collection/<int>")
      .methods("PUT"_method)([](const crow::request &req, int id) {
        auto x = crow::json::load(req.body);
        if (!x) {
          return crow::response(400, "Invalid JSON");
        }

        SqliteCollectionRepository repo;
        auto existing = repo.findById(id);
        if (!existing) {
          return crow::response(404, "Item not found");
        }

        domain::CollectionItem item = *existing;

        if (x.has("title"))
          item.title = x["title"].s();
        if (x.has("purchase_price"))
          item.purchase_price = x["purchase_price"].d();
        if (x.has("is_uhd_4k"))
          item.is_uhd_4k = x["is_uhd_4k"].b();
        if (x.has("notes"))
          item.notes = x["notes"].s();
        if (x.has("format")) {
          // If we add format editing later, handle it here.
          // Currently logic assumes is_uhd_4k boolean.
        }

        // TMDb/IMDb integration fields with validation
        updateMetadataFields(x, item.tmdb_id, item.imdb_id, 
                           item.tmdb_rating, item.trailer_key);

        // Edition & bonus features fields
        updateEditionFields(x, item.edition_type, item.has_slipcover,
                          item.has_digital_copy, item.bonus_features);

        if (repo.update(item)) {
          return crow::response(200);
        }

        return crow::response(500, "Failed to update item");
      });

  // Delete collection item
  CROW_ROUTE(app_, "/api/collection/<int>")
      .methods("DELETE"_method)([this](int id) {
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
      });
}

void WebFrontend::setupReleaseCalendarRoutes() {
  using namespace repositories;

  // Get all release calendar items (paginated)
  CROW_ROUTE(app_, "/api/release-calendar")
      .methods("GET"_method)([this](const crow::request &req) {
        SqliteReleaseCalendarRepository repo;

        int page = 1;
        int page_size = 20;

        if (req.url_params.get("page")) {
          page = std::stoi(req.url_params.get("page"));
        }
        if (req.url_params.get("size")) {
          page_size = std::stoi(req.url_params.get("size"));
        }

        domain::PaginationParams params;
        params.page = page;
        params.page_size = page_size;

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
      });

  // Get release calendar items by date range
  CROW_ROUTE(app_, "/api/release-calendar/range")
      .methods("GET"_method)([this](const crow::request &req) {
        SqliteReleaseCalendarRepository repo;

        std::string start_date =
            req.url_params.get("start") ? req.url_params.get("start") : "";
        std::string end_date =
            req.url_params.get("end") ? req.url_params.get("end") : "";

        if (start_date.empty() || end_date.empty()) {
          return crow::response(400, "Missing start or end date");
        }

        // Parse dates (simple implementation)
        std::tm start_tm = {};
        std::tm end_tm = {};
        std::istringstream start_ss(start_date);
        std::istringstream end_ss(end_date);

        // Validate that date parsing succeeds
        if (!(start_ss >> std::get_time(&start_tm, "%Y-%m-%d")) ||
            !(end_ss >> std::get_time(&end_tm, "%Y-%m-%d"))) {
          return crow::response(
              400, "Invalid start or end date format, expected YYYY-MM-DD");
        }

        auto start_time_t = std::mktime(&start_tm);
        auto end_time_t = std::mktime(&end_tm);

        if (start_time_t == static_cast<std::time_t>(-1) ||
            end_time_t == static_cast<std::time_t>(-1)) {
          return crow::response(400, "Invalid start or end date value");
        }

        auto start_tp = std::chrono::system_clock::from_time_t(start_time_t);
        auto end_tp = std::chrono::system_clock::from_time_t(end_time_t);
        auto items = repo.findByDateRange(start_tp, end_tp);

        crow::json::wvalue response;
        response["items"] = crow::json::wvalue::list();
        response["count"] = items.size();

        for (size_t i = 0; i < items.size(); ++i) {
          response["items"][i] = releaseCalendarItemToJson(items[i]);
        }

        return crow::response(200, response);
      });

  // Add release calendar item manually
  CROW_ROUTE(app_, "/api/release-calendar")
      .methods("POST"_method)([this](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body) {
          return crow::response(400, "Invalid JSON");
        }

        SqliteReleaseCalendarRepository repo;

        domain::ReleaseCalendarItem item;
        item.title = body["title"].s();
        item.format = body.has("format") ? std::string(body["format"].s())
                                         : std::string("Blu-ray");
        item.studio = body.has("studio") ? std::string(body["studio"].s())
                                         : std::string("");
        item.product_url = body.has("product_url")
                               ? std::string(body["product_url"].s())
                               : std::string("");
        item.image_url = body.has("image_url")
                             ? std::string(body["image_url"].s())
                             : std::string("");
        item.is_uhd_4k = body.has("is_uhd_4k") ? body["is_uhd_4k"].b() : false;
        item.price = body.has("price") ? body["price"].d() : 0.0;
        item.notes = body.has("notes") ? std::string(body["notes"].s())
                                       : std::string("");

        // Parse release date
        if (body.has("release_date")) {
          std::string date_str = body["release_date"].s();
          std::tm tm = {};
          std::istringstream ss(date_str);
          ss >> std::get_time(&tm, "%Y-%m-%d");
          if (ss.fail()) {
            return crow::response(
                400, "Invalid release_date format, expected YYYY-MM-DD");
          }
          item.release_date =
              std::chrono::system_clock::from_time_t(std::mktime(&tm));
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
      });

  // Delete release calendar item
  CROW_ROUTE(app_, "/api/release-calendar/<int>")
      .methods("DELETE"_method)([this](int id) {
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
      });
}

void WebFrontend::setupTagRoutes() {
  // Get all tags
  CROW_ROUTE(app_, "/api/tags")
      .methods("GET"_method)([]() {
        SqliteTagRepository repo;
        auto tags = repo.findAll();

        crow::json::wvalue response;
        response["tags"] = crow::json::wvalue::list();

        for (size_t i = 0; i < tags.size(); ++i) {
          crow::json::wvalue tag_json;
          tag_json["id"] = tags[i].id;
          tag_json["name"] = tags[i].name;
          tag_json["color"] = tags[i].color;
          response["tags"][i] = std::move(tag_json);
        }

        return crow::response(200, response);
      });

  // Create a new tag
  CROW_ROUTE(app_, "/api/tags")
      .methods("POST"_method)([this](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body) {
          return crow::response(400, "Invalid JSON");
        }

        if (!body.has("name")) {
          return crow::response(400, "Missing required field: name");
        }

        std::string tag_name = body["name"].s();
        if (!validation::isValidTagName(tag_name)) {
          return crow::response(400, "Invalid tag name (empty, too long, or contains invalid characters)");
        }

        SqliteTagRepository repo;

        domain::Tag tag;
        tag.name = tag_name;
        
        // Validate and sanitize color
        std::string color = body.has("color") ? std::string(body["color"].s()) 
                                              : std::string("#667eea");
        tag.color = validation::sanitizeColor(color);

        int id = repo.add(tag);
        if (id > 0) {
          tag.id = id;

          crow::json::wvalue json;
          json["id"] = tag.id;
          json["name"] = tag.name;
          json["color"] = tag.color;

          // Broadcast update
          crow::json::wvalue ws_msg;
          ws_msg["type"] = "tag_added";
          ws_msg["tag"] = crow::json::wvalue(json);
          broadcastUpdate(ws_msg.dump());

          return crow::response(201, json);
        }

        return crow::response(500, "Failed to create tag (name may already exist)");
      });

  // Update a tag
  CROW_ROUTE(app_, "/api/tags/<int>")
      .methods("PUT"_method)([this](const crow::request &req, int id) {
        auto body = crow::json::load(req.body);
        if (!body) {
          return crow::response(400, "Invalid JSON");
        }

        SqliteTagRepository repo;
        auto existing = repo.findById(id);
        if (!existing) {
          return crow::response(404, "Tag not found");
        }

        domain::Tag tag = *existing;
        if (body.has("name")) {
          std::string tag_name = body["name"].s();
          if (!validation::isValidTagName(tag_name)) {
            return crow::response(400, "Invalid tag name (empty, too long, or contains invalid characters)");
          }
          tag.name = tag_name;
        }
        if (body.has("color")) {
          tag.color = validation::sanitizeColor(body["color"].s());
        }

        if (repo.update(tag)) {
          crow::json::wvalue json;
          json["id"] = tag.id;
          json["name"] = tag.name;
          json["color"] = tag.color;
          
          // Broadcast update via WebSocket
          crow::json::wvalue ws_msg;
          ws_msg["type"] = "tag_updated";
          ws_msg["tag"] = crow::json::wvalue(json);
          broadcastUpdate(ws_msg.dump());
          
          return crow::response(200, json);
        }

        return crow::response(500, "Failed to update tag");
      });

  // Delete a tag
  CROW_ROUTE(app_, "/api/tags/<int>")
      .methods("DELETE"_method)([this](int id) {
        SqliteTagRepository repo;
        if (repo.remove(id)) {
          crow::json::wvalue ws_msg;
          ws_msg["type"] = "tag_deleted";
          ws_msg["id"] = id;
          broadcastUpdate(ws_msg.dump());

          return crow::response(200, "Tag deleted");
        }
        return crow::response(404, "Tag not found");
      });

  // Generic handler factory for adding/removing tags to/from items
  auto createTagAssignmentHandler = [this](const std::string &item_type, bool is_add) {
    return [this, item_type, is_add](int item_id, int tag_id) {
      SqliteTagRepository repo;
      bool success = false;
      if (is_add) {
        success = repo.addTagToItem(tag_id, item_id, item_type);
      } else {
        success = repo.removeTagFromItem(tag_id, item_id, item_type);
      }

      if (success) {
        crow::json::wvalue ws_msg;
        ws_msg["type"] =
            fmt::format("{}_tag_{}", item_type, is_add ? "added" : "removed");
        ws_msg["item_id"] = item_id;
        ws_msg["tag_id"] = tag_id;
        broadcastUpdate(ws_msg.dump());

        const char *success_message =
            is_add ? "Tag added to item" : "Tag removed from item";
        return crow::response(200, success_message);
      }

      const char *failure_message =
          is_add ? "Failed to add tag" : "Failed to remove tag";
      return crow::response(500, failure_message);
    };
  };

  // Add tag to wishlist item
  CROW_ROUTE(app_, "/api/wishlist/<int>/tags/<int>")
      .methods("POST"_method)(createTagAssignmentHandler("wishlist", true));

  // Remove tag from wishlist item
  CROW_ROUTE(app_, "/api/wishlist/<int>/tags/<int>")
      .methods("DELETE"_method)(createTagAssignmentHandler("wishlist", false));

  // Add tag to collection item
  CROW_ROUTE(app_, "/api/collection/<int>/tags/<int>")
      .methods("POST"_method)(createTagAssignmentHandler("collection", true));

  // Remove tag from collection item
  CROW_ROUTE(app_, "/api/collection/<int>/tags/<int>")
      .methods("DELETE"_method)(createTagAssignmentHandler("collection", false));
}

void WebFrontend::setupActionRoutes() {
  // Trigger scrape now
  CROW_ROUTE(app_, "/api/action/scrape").methods("POST"_method)([this]() {
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
    } catch (const std::exception &e) {
      crow::json::wvalue response;
      response["success"] = false;
      response["error"] = e.what();
      return crow::response(500, response);
    }
  });

  // Trigger release calendar scrape
  CROW_ROUTE(app_, "/api/scrape-calendar").methods("POST"_method)([this]() {
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
    } catch (const std::exception &e) {
      crow::json::wvalue response;
      response["success"] = false;
      response["error"] = e.what();
      return crow::response(500, response);
    }
  });

  // Get dashboard stats
  // Get dashboard stats
  CROW_ROUTE(app_, "/api/stats").methods("GET"_method)([this]() {
    SqliteWishlistRepository wishlist_repo;
    SqliteCollectionRepository collection_repo;

    crow::json::wvalue response;
    response["wishlist_count"] = wishlist_repo.count();
    response["collection_count"] = collection_repo.count();

    // Count in-stock items
    auto wishlist = wishlist_repo.findAll();
    int in_stock_count = 0;
    int uhd_4k_count = 0;
    for (const auto &item : wishlist) {
      if (item.in_stock)
        in_stock_count++;
      if (item.is_uhd_4k)
        uhd_4k_count++;
    }
    response["in_stock_count"] = in_stock_count;
    response["uhd_4k_count"] = uhd_4k_count;

    // Scrape Progress
    auto progress = scheduler_->getScrapeProgress();
    response["scraping_active"] = progress.is_active;

    crow::json::wvalue progress_json;
    progress_json["processed"] = progress.processed;
    progress_json["total"] = progress.total;
    response["scrape_progress"] = std::move(progress_json);

    return crow::response(200, response);
  });
}

void WebFrontend::setupEnrichmentRoutes() {
  // Enrich single wishlist item
  CROW_ROUTE(app_, "/api/wishlist/<int>/enrich")
      .methods("POST"_method)([this](int id) {
        SqliteWishlistRepository repo;
        auto item_opt = repo.findById(id);

        if (!item_opt) {
          crow::json::wvalue error_response;
          error_response["success"] = false;
          error_response["error"] = "Wishlist item not found";
          return crow::response(404, error_response);
        }

        auto item = *item_opt;

        // Create enrichment service
        application::enrichment::TmdbEnrichmentService service;

        if (!service.isEnabled()) {
          crow::json::wvalue error_response;
          error_response["success"] = false;
          error_response["error"] =
              "TMDb API key not configured. Please add your API key in Settings.";
          return crow::response(400, error_response);
        }

        // Enrich the item
        auto result = service.enrichWishlistItem(item);

        if (result.success) {
          // Save enriched item
          if (!repo.update(item)) {
            crow::json::wvalue error_response;
            error_response["success"] = false;
            error_response["error"] = "Failed to save enriched item";
            return crow::response(500, error_response);
          }

          // Broadcast update to WebSocket clients
          crow::json::wvalue ws_msg;
          ws_msg["type"] = "wishlist_updated";
          ws_msg["item"] = wishlistItemToJson(item);
          broadcastUpdate(ws_msg.dump());

          // Return success response
          crow::json::wvalue response;
          response["success"] = true;
          response["tmdb_id"] = result.tmdb_id;
          response["imdb_id"] = result.imdb_id;
          response["tmdb_rating"] = result.tmdb_rating;
          response["trailer_key"] = result.trailer_key;
          response["confidence"] = result.confidence_score;

          return crow::response(200, response);
        } else {
          // Return failure response
          crow::json::wvalue error_response;
          error_response["success"] = false;
          error_response["error"] = result.error_message;
          error_response["confidence"] = result.confidence_score;

          return crow::response(200, error_response);
        }
      });

  // Enrich single collection item
  CROW_ROUTE(app_, "/api/collection/<int>/enrich")
      .methods("POST"_method)([this](int id) {
        SqliteCollectionRepository repo;
        auto item_opt = repo.findById(id);

        if (!item_opt) {
          crow::json::wvalue error_response;
          error_response["success"] = false;
          error_response["error"] = "Collection item not found";
          return crow::response(404, error_response);
        }

        auto item = *item_opt;

        // Create enrichment service
        application::enrichment::TmdbEnrichmentService service;

        if (!service.isEnabled()) {
          crow::json::wvalue error_response;
          error_response["success"] = false;
          error_response["error"] =
              "TMDb API key not configured. Please add your API key in Settings.";
          return crow::response(400, error_response);
        }

        // Enrich the item
        auto result = service.enrichCollectionItem(item);

        if (result.success) {
          // Save enriched item
          if (!repo.update(item)) {
            crow::json::wvalue error_response;
            error_response["success"] = false;
            error_response["error"] = "Failed to save enriched item";
            return crow::response(500, error_response);
          }

          // Broadcast update to WebSocket clients
          crow::json::wvalue ws_msg;
          ws_msg["type"] = "collection_updated";
          ws_msg["item"] = collectionItemToJson(item);
          broadcastUpdate(ws_msg.dump());

          // Return success response
          crow::json::wvalue response;
          response["success"] = true;
          response["tmdb_id"] = result.tmdb_id;
          response["imdb_id"] = result.imdb_id;
          response["tmdb_rating"] = result.tmdb_rating;
          response["trailer_key"] = result.trailer_key;
          response["confidence"] = result.confidence_score;

          return crow::response(200, response);
        } else {
          // Return failure response
          crow::json::wvalue error_response;
          error_response["success"] = false;
          error_response["error"] = result.error_message;
          error_response["confidence"] = result.confidence_score;

          return crow::response(200, error_response);
        }
      });

  // Bulk enrich wishlist items (async)
  CROW_ROUTE(app_, "/api/enrich/bulk")
      .methods("POST"_method)([this](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body) {
          return crow::response(400, "Invalid JSON");
        }

        if (!body.has("item_type") || !body.has("item_ids")) {
          crow::json::wvalue error_response;
          error_response["success"] = false;
          error_response["error"] =
              "Missing required fields: item_type, item_ids";
          return crow::response(400, error_response);
        }

        std::string item_type = body["item_type"].s();
        std::vector<int> item_ids;

        // Parse item IDs array
        auto ids_array = body["item_ids"];
        for (size_t i = 0; i < ids_array.size(); ++i) {
          item_ids.push_back(ids_array[i].i());
        }

        // Launch async enrichment in background thread
        std::thread([this, item_type, item_ids]() {
          application::enrichment::TmdbEnrichmentService service;

          auto progress_callback =
              [this](const application::enrichment::BulkEnrichmentProgress
                         &progress) {
                // Broadcast progress via WebSocket
                crow::json::wvalue ws_msg;
                ws_msg["type"] = "enrichment_progress";
                ws_msg["processed"] = progress.processed;
                ws_msg["total"] = progress.total;
                ws_msg["successful"] = progress.successful;
                ws_msg["failed"] = progress.failed;
                ws_msg["is_active"] = progress.is_active;
                broadcastUpdate(ws_msg.dump());
              };

          application::enrichment::BulkEnrichmentProgress final_progress;

          if (item_type == "wishlist") {
            final_progress =
                service.enrichMultipleWishlistItems(item_ids, progress_callback);
          } else if (item_type == "collection") {
            final_progress = service.enrichMultipleCollectionItems(
                item_ids, progress_callback);
          }

          // Broadcast completion
          crow::json::wvalue ws_msg;
          ws_msg["type"] = "enrichment_completed";
          ws_msg["processed"] = final_progress.processed;
          ws_msg["successful"] = final_progress.successful;
          ws_msg["failed"] = final_progress.failed;
          broadcastUpdate(ws_msg.dump());
        }).detach();

        // Return immediate response
        crow::json::wvalue response;
        response["started"] = true;
        response["total"] = static_cast<int>(item_ids.size());
        return crow::response(200, response);
      });

  // Get enrichment progress
  CROW_ROUTE(app_, "/api/enrich/progress").methods("GET"_method)([]() {
    application::enrichment::TmdbEnrichmentService service;
    auto progress = service.getCurrentProgress();

    crow::json::wvalue response;
    response["total"] = progress.total;
    response["processed"] = progress.processed;
    response["successful"] = progress.successful;
    response["failed"] = progress.failed;
    response["is_active"] = progress.is_active;
    response["current_item_id"] = progress.current_item_id;

    return crow::response(200, response);
  });

  // Auto-enrich all unenriched items
  CROW_ROUTE(app_, "/api/enrich/auto")
      .methods("POST"_method)([this](const crow::request &req) {
        auto body = crow::json::load(req.body);
        std::string item_type = "wishlist"; // Default

        if (body && body.has("item_type")) {
          item_type = body["item_type"].s();
        }

        // Find all items with tmdb_id = 0
        std::vector<int> unenriched_ids;

        if (item_type == "wishlist") {
          SqliteWishlistRepository repo;
          domain::PaginationParams params;
          params.page = 1;
          params.page_size = 10000; // Get all items

          auto result = repo.findAll(params);
          for (const auto &item : result.items) {
            if (item.tmdb_id == 0) {
              unenriched_ids.push_back(item.id);
            }
          }
        } else if (item_type == "collection") {
          SqliteCollectionRepository repo;
          domain::PaginationParams params;
          params.page = 1;
          params.page_size = 10000; // Get all items

          auto result = repo.findAll(params);
          for (const auto &item : result.items) {
            if (item.tmdb_id == 0) {
              unenriched_ids.push_back(item.id);
            }
          }
        }

        if (unenriched_ids.empty()) {
          crow::json::wvalue response;
          response["started"] = false;
          response["total"] = 0;
          response["message"] = "No unenriched items found";
          return crow::response(200, response);
        }

        // Launch async enrichment
        std::thread([this, item_type, unenriched_ids]() {
          application::enrichment::TmdbEnrichmentService service;

          auto progress_callback =
              [this](const application::enrichment::BulkEnrichmentProgress
                         &progress) {
                crow::json::wvalue ws_msg;
                ws_msg["type"] = "enrichment_progress";
                ws_msg["processed"] = progress.processed;
                ws_msg["total"] = progress.total;
                ws_msg["successful"] = progress.successful;
                ws_msg["failed"] = progress.failed;
                ws_msg["is_active"] = progress.is_active;
                broadcastUpdate(ws_msg.dump());
              };

          application::enrichment::BulkEnrichmentProgress final_progress;

          if (item_type == "wishlist") {
            final_progress = service.enrichMultipleWishlistItems(unenriched_ids,
                                                                  progress_callback);
          } else if (item_type == "collection") {
            final_progress = service.enrichMultipleCollectionItems(
                unenriched_ids, progress_callback);
          }

          // Broadcast completion
          crow::json::wvalue ws_msg;
          ws_msg["type"] = "enrichment_completed";
          ws_msg["processed"] = final_progress.processed;
          ws_msg["successful"] = final_progress.successful;
          ws_msg["failed"] = final_progress.failed;
          broadcastUpdate(ws_msg.dump());
        }).detach();

        crow::json::wvalue response;
        response["started"] = true;
        response["total"] = static_cast<int>(unenriched_ids.size());
        return crow::response(200, response);
      });
}

void WebFrontend::setupSettingsRoutes() {
  // Get settings
  CROW_ROUTE(app_, "/api/settings").methods("GET"_method)([]() {
    auto &config = ConfigManager::instance();

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

    // TMDb settings (never return the actual API key, only if it's configured)
    std::string tmdb_key = config.get("tmdb_api_key", "");
    response["tmdb_api_key_configured"] = !tmdb_key.empty();
    response["tmdb_auto_enrich"] = config.getInt("tmdb_auto_enrich", 0) > 0;
    response["tmdb_enrich_on_add"] = config.getInt("tmdb_enrich_on_add", 1) > 0;

    return crow::response(200, response);
  });

  // Update settings
  CROW_ROUTE(app_, "/api/settings")
      .methods("PUT"_method)([](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body) {
          return crow::response(400, "Invalid JSON");
        }

        auto &config = ConfigManager::instance();

        if (body.has("scrape_delay_seconds")) {
          config.set("scrape_delay_seconds",
                     std::to_string(body["scrape_delay_seconds"].i()));
        }
        if (body.has("discord_webhook_url")) {
          config.set("discord_webhook_url",
                     std::string(body["discord_webhook_url"].s()));
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

        // TMDb settings
        if (body.has("tmdb_api_key")) {
          config.set("tmdb_api_key", std::string(body["tmdb_api_key"].s()));
        }
        if (body.has("tmdb_auto_enrich")) {
          config.set("tmdb_auto_enrich", body["tmdb_auto_enrich"].b() ? "1" : "0");
        }
        if (body.has("tmdb_enrich_on_add")) {
          config.set("tmdb_enrich_on_add",
                     body["tmdb_enrich_on_add"].b() ? "1" : "0");
        }

        return crow::response(200, "Settings updated");
      });
}

void WebFrontend::setupStaticRoutes() {
  // Serve cached images
  CROW_ROUTE(app_, "/cache/<string>")
  ([](const std::string &filename) {
    auto &config = ConfigManager::instance();
    const std::string cache_dir = config.get("cache_directory", "./cache");
    const std::filesystem::path file_path =
        std::filesystem::path(cache_dir) / filename;

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
    if (filename.length() >= 4 &&
        filename.rfind(".png") == filename.length() - 4)
      content_type = "image/png";
    else if (filename.length() >= 4 &&
             filename.rfind(".gif") == filename.length() - 4)
      content_type = "image/gif";
    else if (filename.length() >= 5 &&
             filename.rfind(".webp") == filename.length() - 5)
      content_type = "image/webp";

    crow::response res(content);
    res.set_header("Content-Type", content_type);
    res.set_header("Cache-Control", "public, max-age=31536000");
    return res;
  });
}

std::string WebFrontend::timePointToString(
    const std::chrono::system_clock::time_point &tp) {
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

crow::json::wvalue
WebFrontend::wishlistItemToJson(const domain::WishlistItem &item) {
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
  json["title_locked"] = item.title_locked;
  json["created_at"] = timePointToString(item.created_at);
  json["last_checked"] = timePointToString(item.last_checked);

  // TMDb/IMDb integration
  json["tmdb_id"] = item.tmdb_id;
  json["imdb_id"] = item.imdb_id;
  json["tmdb_rating"] = item.tmdb_rating;
  json["trailer_key"] = item.trailer_key;

  // Edition & bonus features
  json["edition_type"] = item.edition_type;
  json["has_slipcover"] = item.has_slipcover;
  json["has_digital_copy"] = item.has_digital_copy;
  json["bonus_features"] = item.bonus_features;

  // Get tags for this item
  populateTagJson(json, item.id, "wishlist");

  return json;
}

crow::json::wvalue
WebFrontend::collectionItemToJson(const domain::CollectionItem &item) {
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

  // TMDb/IMDb integration
  json["tmdb_id"] = item.tmdb_id;
  json["imdb_id"] = item.imdb_id;
  json["tmdb_rating"] = item.tmdb_rating;
  json["trailer_key"] = item.trailer_key;

  // Edition & bonus features
  json["edition_type"] = item.edition_type;
  json["has_slipcover"] = item.has_slipcover;
  json["has_digital_copy"] = item.has_digital_copy;
  json["bonus_features"] = item.bonus_features;

  // Get tags for this item
  populateTagJson(json, item.id, "collection");

  return json;
}

crow::json::wvalue WebFrontend::releaseCalendarItemToJson(
    const domain::ReleaseCalendarItem &item) {
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

std::string WebFrontend::renderSPA() { return renderer_->renderSPA(); }

} // namespace bluray::presentation
