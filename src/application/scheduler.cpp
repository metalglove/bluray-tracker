#include "scheduler.hpp"
#include "../infrastructure/config_manager.hpp"
#include "../infrastructure/logger.hpp"
#include "../infrastructure/repositories/price_history_repository.hpp"
#include "scraper/scraper.hpp"
#include <format>
#include <thread>

namespace bluray::application {

using namespace infrastructure;
using namespace repositories;

Scheduler::Scheduler() {
    // Load configuration
    auto& config = ConfigManager::instance();
    scrape_delay_seconds_ = config.getInt("scrape_delay_seconds", 8);

    const std::string cache_dir = config.get("cache_directory", "./cache");
    image_cache_ = std::make_unique<ImageCache>(cache_dir);

    Logger::instance().info(
        std::format("Scheduler initialized (delay: {}s)", scrape_delay_seconds_)
    );
}

void Scheduler::addNotifier(std::shared_ptr<notifier::INotifier> notifier) {
    if (notifier && notifier->isConfigured()) {
        change_detector_.addObserver(notifier);
        Logger::instance().info("Notifier added to scheduler");
    }
}

int Scheduler::getScrapeDelay() const {
    return scrape_delay_seconds_;
}

int Scheduler::runOnce() {
    Logger::instance().info("Starting scrape run");

    SqliteWishlistRepository repo;
    auto wishlist_items = repo.findAll();

    if (wishlist_items.empty()) {
        Logger::instance().info("No items in wishlist to scrape");
        return 0;
    }

    Logger::instance().info(
        std::format("Scraping {} wishlist items", wishlist_items.size())
    );

    int processed_count = 0;
    int success_count = 0;
    int error_count = 0;

    for (const auto& item : wishlist_items) {
        Logger::instance().debug(std::format("Scraping: {}", item.url));

        // Scrape product
        auto result = scrapeProduct(item.url);

        if (result.success) {
            // Cache image if available
            if (!result.product.image_url.empty()) {
                auto cached_path = image_cache_->cacheImage(result.product.image_url);
                if (cached_path) {
                    result.product.local_image_path = *cached_path;
                }
            }

            // Update wishlist item and detect changes
            updateWishlistItem(repo, item, result.product);
            ++success_count;
        } else {
            Logger::instance().warning(
                std::format("Failed to scrape {}: {}", item.url, result.error_message)
            );
            ++error_count;
        }

        ++processed_count;

        // Delay between scrapes (except for last item)
        if (processed_count < static_cast<int>(wishlist_items.size())) {
            Logger::instance().debug(
                std::format("Waiting {}s before next scrape", scrape_delay_seconds_)
            );
            std::this_thread::sleep_for(std::chrono::seconds(scrape_delay_seconds_));
        }
    }

    Logger::instance().info(
        std::format("Scrape run completed: {} processed, {} succeeded, {} failed",
                   processed_count, success_count, error_count)
    );

    return processed_count;
}

Scheduler::ScrapeResult Scheduler::scrapeProduct(const std::string& url) {
    ScrapeResult result;

    // Create appropriate scraper
    auto scraper = scraper::ScraperFactory::create(url);
    if (!scraper) {
        result.error_message = "No scraper available for URL";
        return result;
    }

    // Scrape
    try {
        auto product = scraper->scrape(url);
        if (product) {
            result.success = true;
            result.product = *product;
        } else {
            result.error_message = "Scraping returned no data";
        }
    } catch (const std::exception& e) {
        result.error_message = std::format("Exception: {}", e.what());
    }

    return result;
}

void Scheduler::updateWishlistItem(
    SqliteWishlistRepository& repo,
    const domain::WishlistItem& old_item,
    const domain::Product& product
) {
    // Create updated item
    domain::WishlistItem updated_item = old_item;
    updated_item.title = product.title;
    updated_item.current_price = product.price;
    updated_item.in_stock = product.in_stock;
    updated_item.is_uhd_4k = product.is_uhd_4k;
    updated_item.image_url = product.image_url;
    updated_item.local_image_path = product.local_image_path;
    updated_item.source = product.source;
    updated_item.last_checked = product.last_updated;

    // Detect changes before updating
    auto changes = change_detector_.detectChanges(old_item, updated_item);

    // Update in database
    if (!repo.update(updated_item)) {
        Logger::instance().error(
            std::format("Failed to update wishlist item: {}", updated_item.url)
        );
        return;
    }

    // Insert price history entry
    SqlitePriceHistoryRepository price_history_repo;
    domain::PriceHistoryEntry history_entry{
        .wishlist_id = updated_item.id,
        .price = updated_item.current_price,
        .in_stock = updated_item.in_stock,
        .recorded_at = updated_item.last_checked
    };
    price_history_repo.add(history_entry);

    // Log changes
    if (!changes.empty()) {
        Logger::instance().info(
            std::format("Detected {} change(s) for: {}", changes.size(), updated_item.title)
        );
        for (const auto& change : changes) {
            Logger::instance().info(std::format("  - {}", change.describe()));
        }
    }
}

} // namespace bluray::application
