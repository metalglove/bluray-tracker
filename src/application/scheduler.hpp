#pragma once

#include "../domain/models.hpp"
#include "../domain/change_detector.hpp"
#include "../infrastructure/repositories/wishlist_repository.hpp"
#include "../infrastructure/repositories/release_calendar_repository.hpp"
#include "../infrastructure/image_cache.hpp"
#include "scraper/scraper.hpp"
#include "scraper/bluray_com_scraper.hpp"
#include "notifier/notifier.hpp"
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

namespace bluray::application {

/**
 * Scheduler orchestrates the scraping and notification process
 */
class Scheduler {
public:
    Scheduler();

    /**
     * Run scraping once for all wishlist items
     * Returns number of items processed
     */
    int runOnce();

    /**
     * Scrape release calendar and update database
     * Returns number of releases found
     */
    int scrapeReleaseCalendar();

    /**
     * Add notifier to receive change notifications
     */
    void addNotifier(std::shared_ptr<notifier::INotifier> notifier);

    /**
     * Get scrape delay in seconds from config
     */
    int getScrapeDelay() const;

private:
    struct ScrapeResult {
        bool success{false};
        domain::Product product;
        std::string error_message;
    };

    ScrapeResult scrapeProduct(const std::string& url);
    void updateWishlistItem(
        infrastructure::repositories::SqliteWishlistRepository& repo,
        const domain::WishlistItem& old_item,
        const domain::Product& product
    );

    domain::ChangeDetector change_detector_;
    std::unique_ptr<infrastructure::ImageCache> image_cache_;
    int scrape_delay_seconds_{8};
};

} // namespace bluray::application
