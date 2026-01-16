#pragma once

#include "../application/scheduler.hpp"
#include "../infrastructure/repositories/wishlist_repository.hpp"
#include "../infrastructure/repositories/collection_repository.hpp"
#include <crow.h>
#include <memory>
#include <mutex>
#include <set>

namespace bluray::presentation {

/**
 * Web frontend using Crow framework with WebSocket support
 */
class WebFrontend {
public:
    explicit WebFrontend(std::shared_ptr<application::Scheduler> scheduler);

    /**
     * Start web server
     */
    void run(int port = 8080);

    /**
     * Stop web server
     */
    void stop();

    /**
     * Broadcast update to all connected WebSocket clients
     */
    void broadcastUpdate(const std::string& message);

private:
    void setupRoutes();

    // API endpoints
    void setupWishlistRoutes();
    void setupCollectionRoutes();
    void setupActionRoutes();
    void setupStaticRoutes();
    void setupWebSocketRoute();
    void setupSettingsRoutes();

    // HTML rendering
    std::string renderSPA();

    // Helper methods
    crow::json::wvalue wishlistItemToJson(const domain::WishlistItem& item);
    crow::json::wvalue collectionItemToJson(const domain::CollectionItem& item);
    std::string timePointToString(const std::chrono::system_clock::time_point& tp);

    crow::SimpleApp app_;
    std::shared_ptr<application::Scheduler> scheduler_;

    // WebSocket connections
    std::mutex ws_mutex_;
    std::set<crow::websocket::connection*> ws_connections_;
};

} // namespace bluray::presentation
