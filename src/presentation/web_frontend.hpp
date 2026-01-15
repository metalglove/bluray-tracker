#pragma once

#include "../application/scheduler.hpp"
#include "../infrastructure/repositories/wishlist_repository.hpp"
#include "../infrastructure/repositories/collection_repository.hpp"
#include <crow.h>
#include <memory>

namespace bluray::presentation {

/**
 * Web frontend using Crow framework
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

private:
    void setupRoutes();

    // API endpoints
    void setupWishlistRoutes();
    void setupCollectionRoutes();
    void setupActionRoutes();
    void setupStaticRoutes();

    // HTML pages
    std::string renderHomePage();
    std::string renderWishlistPage();
    std::string renderCollectionPage();

    // Helper methods
    crow::json::wvalue wishlistItemToJson(const domain::WishlistItem& item);
    crow::json::wvalue collectionItemToJson(const domain::CollectionItem& item);

    crow::SimpleApp app_;
    std::shared_ptr<application::Scheduler> scheduler_;
};

} // namespace bluray::presentation
