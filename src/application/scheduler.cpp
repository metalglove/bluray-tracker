#include "scheduler.hpp"
#include "../infrastructure/config_manager.hpp"
#include "../infrastructure/logger.hpp"
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

int Scheduler::scrapeReleaseCalendar() {
    auto& config = ConfigManager::instance();

    // Check if calendar scraping is enabled
    const bool enabled = config.getInt("bluray_calendar_enabled", 1) != 0;
    if (!enabled) {
        Logger::instance().info("Release calendar scraping is disabled");
        return 0;
    }

    const std::string calendar_url = config.get(
        "bluray_calendar_url",
        "https://www.blu-ray.com/movies/releasedates.php"
    );
    const int days_ahead = config.getInt("bluray_calendar_days_ahead", 90);

    Logger::instance().info(
        std::format("Scraping release calendar from: {} ({} days ahead)", calendar_url, days_ahead)
    );

    // Create scraper and fetch calendar
    scraper::BluRayComScraper scraper;
    std::vector<domain::ReleaseCalendarItem> releases;

    try {
        releases = scraper.scrapeReleaseCalendar(calendar_url);
    } catch (const std::exception& e) {
        Logger::instance().error(
            std::format("Failed to scrape release calendar: {}", e.what())
        );
        return 0;
    }

    if (releases.empty()) {
        Logger::instance().warning("No releases found in calendar");
        return 0;
    }

    Logger::instance().info(std::format("Found {} releases", releases.size()));

    // Filter releases by date range (only keep upcoming releases within configured days)
    auto now = std::chrono::system_clock::now();
    auto cutoff_date = now + std::chrono::hours(24) * static_cast<std::chrono::hours::rep>(days_ahead);

    std::vector<domain::ReleaseCalendarItem> filtered_releases;
    for (auto& release : releases) {
        if (release.release_date >= now && release.release_date <= cutoff_date) {
            // Cache image if available
            if (!release.image_url.empty()) {
                auto cached_path = image_cache_->cacheImage(release.image_url);
                if (cached_path) {
                    release.local_image_path = *cached_path;
                }
            }
            filtered_releases.push_back(release);
        }
    }

    Logger::instance().info(
        std::format("Filtered to {} upcoming releases (within {} days)",
                   filtered_releases.size(), days_ahead)
    );

    // Update database
    SqliteReleaseCalendarRepository repo;

    // Clear old releases (older than today)
    repo.removeOlderThan(now);

    int added_count = 0;
    int updated_count = 0;

    for (const auto& release : filtered_releases) {
        // Check if release already exists (by URL if available)
        if (!release.product_url.empty()) {
            auto existing = repo.findByUrl(release.product_url);
            if (existing) {
                // Update existing release
                auto updated = *existing;
                updated.title = release.title;
                updated.release_date = release.release_date;
                updated.format = release.format;
                updated.studio = release.studio;
                updated.image_url = release.image_url;
                updated.local_image_path = release.local_image_path;
                updated.is_uhd_4k = release.is_uhd_4k;
                updated.is_preorder = release.is_preorder;
                updated.price = release.price;
                updated.last_updated = std::chrono::system_clock::now();

                if (repo.update(updated)) {
                    ++updated_count;
                }
                continue;
            }
        }

        // Add new release
        if (repo.add(release) > 0) {
            ++added_count;
        }
    }

    Logger::instance().info(
        std::format("Calendar update complete: {} added, {} updated",
                   added_count, updated_count)
    );

    return static_cast<int>(filtered_releases.size());
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
