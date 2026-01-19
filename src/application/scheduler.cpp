#include "scheduler.hpp"
#include "../infrastructure/config_manager.hpp"
#include "../infrastructure/logger.hpp"
#include "../infrastructure/repositories/price_history_repository.hpp"
#include "../infrastructure/repositories/release_calendar_repository.hpp"
#include "scraper/bluray_com_scraper.hpp"
#include "scraper/scraper.hpp"
#include <algorithm>
#include <fmt/format.h>
#include <future>
#include <thread>
#include <vector>

namespace bluray::application {

using namespace infrastructure;
using namespace repositories;

Scheduler::Scheduler() {
  // Load configuration
  auto &config = infrastructure::ConfigManager::instance();
  delay_seconds_ = config.getInt("scrape_delay_seconds", 8);

  const std::string cache_dir = config.get("cache_directory", "./cache");
  image_cache_ = std::make_unique<ImageCache>(cache_dir);

  Logger::instance().info(
      fmt::format("Scheduler initialized (delay: {}s)", delay_seconds_));
}

void Scheduler::addNotifier(std::shared_ptr<notifier::INotifier> notifier) {
  if (notifier && notifier->isConfigured()) {
    change_detector_.addObserver(notifier);
    Logger::instance().info("Notifier added to scheduler");
  }
}

Scheduler::ScrapeProgress Scheduler::getScrapeProgress() const {
  return {scrape_processed_.load(), scrape_total_.load(), is_running_.load()};
}

int Scheduler::getScrapeDelay() const { return delay_seconds_; }

int Scheduler::runOnce() {
  if (is_running_.exchange(true)) {
    Logger::instance().warning("Scrape already in progress");
    return 0;
  }

  Logger::instance().info("Starting scrape run");

  SqliteWishlistRepository repo;
  auto wishlist_items = repo.findAll();

  if (wishlist_items.empty()) {
    Logger::instance().info("No items in wishlist to scrape");
    is_running_ = false;
    return 0;
  }

  Logger::instance().info(
      fmt::format("Scraping {} wishlist items", wishlist_items.size()));

  scrape_total_ = static_cast<int>(wishlist_items.size());
  scrape_processed_ = 0;

  // Counters for this run
  std::atomic<int> processed_count{0};
  std::atomic<int> success_count{0};
  std::atomic<int> error_count{0};

  // Concurrency limit
  const int kConcurrency = 4;
  std::vector<std::future<void>> futures;

  auto process_item = [&](domain::WishlistItem item) {
    Logger::instance().debug(fmt::format("Scraping: {}", item.url));

    // Scrape product
    auto result = scrapeProduct(item.url);

    if (result.success) {
      // Cache image if available
      if (!result.product.image_url.empty()) {
        // mutex is handled inside ImageCache
        auto cached_path = image_cache_->cacheImage(result.product.image_url);
        if (cached_path) {
          result.product.local_image_path = *cached_path;
        }
      }

      // Update wishlist item
      updateWishlistItem(repo, item, result.product);
      success_count++;
    } else {
      Logger::instance().warning(fmt::format("Failed to scrape {}: {}",
                                             item.url, result.error_message));
      error_count++;
    }
    processed_count++;
    scrape_processed_++;
  };

  for (size_t i = 0; i < wishlist_items.size(); ++i) {
    // Simple limiter: clean up finished futures
    // Remove ready futures
    futures.erase(std::remove_if(futures.begin(), futures.end(),
                                 [](const std::future<void> &f) {
                                   return f.wait_for(std::chrono::seconds(0)) ==
                                          std::future_status::ready;
                                 }),
                  futures.end());

    if (futures.size() >= kConcurrency) {
      // Wait for the oldest one to finish to keep pool size stable
      if (!futures.empty()) {
        futures.front().wait();
        futures.erase(futures.begin());
      }
    }
    futures.push_back(
        std::async(std::launch::async, process_item, wishlist_items[i]));

    // Rate limiting: Throttle the launch rate
    // Start with configured delay divided by concurrency to spread requests out
    // But ensure at least some delay between requests if delay_seconds > 0
    if (delay_seconds_ > 0) {
      int throttle_ms = (delay_seconds_ * 1000) / kConcurrency;
      // Clamp to reasonable minimum if user has configured a delay
      if (throttle_ms < 1000)
        throttle_ms = 1000;

      Logger::instance().debug(
          fmt::format("Throttling request for {}ms", throttle_ms));
      std::this_thread::sleep_for(std::chrono::milliseconds(throttle_ms));
    }
  }

  // Wait for all remaining
  for (auto &f : futures) {
    f.wait();
  }

  is_running_ = false;

  Logger::instance().info(fmt::format(
      "Scrape run completed: {} processed, {} succeeded, {} failed",
      processed_count.load(), success_count.load(), error_count.load()));

  return processed_count.load();
}

int Scheduler::scrapeReleaseCalendar() {
  auto &config = ConfigManager::instance();

  // Check if calendar scraping is enabled
  const bool enabled = config.getInt("bluray_calendar_enabled", 1) != 0;
  if (!enabled) {
    Logger::instance().info("Release calendar scraping is disabled");
    return 0;
  }

  const std::string calendar_url = config.get(
      "bluray_calendar_url", "https://www.blu-ray.com/movies/releasedates.php");
  const int days_ahead = config.getInt("bluray_calendar_days_ahead", 90);

  Logger::instance().info(
      fmt::format("Scraping release calendar from: {} ({} days ahead)",
                  calendar_url, days_ahead));

  // Create scraper and fetch calendar
  scraper::BluRayComScraper scraper;
  std::vector<domain::ReleaseCalendarItem> releases;

  try {
    releases = scraper.scrapeReleaseCalendar(calendar_url);
  } catch (const std::exception &e) {
    Logger::instance().error(
        fmt::format("Failed to scrape release calendar: {}", e.what()));
    return 0;
  }

  if (releases.empty()) {
    Logger::instance().warning("No releases found in calendar");
    return 0;
  }

  Logger::instance().info(fmt::format("Found {} releases", releases.size()));

  // Filter releases by date range (only keep upcoming releases within
  // configured days)
  auto now = std::chrono::system_clock::now();
  auto cutoff_date = now + std::chrono::hours(24) *
                               static_cast<std::chrono::hours::rep>(days_ahead);

  std::vector<domain::ReleaseCalendarItem> filtered_releases;
  for (auto &release : releases) {
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
      fmt::format("Filtered to {} upcoming releases (within {} days)",
                  filtered_releases.size(), days_ahead));

  // Update database
  SqliteReleaseCalendarRepository repo;

  // Clear old releases (older than today)
  repo.removeOlderThan(now);

  int added_count = 0;
  int updated_count = 0;

  for (const auto &release : filtered_releases) {
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
      fmt::format("Calendar update complete: {} added, {} updated", added_count,
                  updated_count));

  return static_cast<int>(filtered_releases.size());
}

Scheduler::ScrapeResult Scheduler::scrapeProduct(const std::string &url) {
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
  } catch (const std::exception &e) {
    result.error_message = fmt::format("Exception: {}", e.what());
  }

  return result;
}

void Scheduler::updateWishlistItem(SqliteWishlistRepository &repo,
                                   const domain::WishlistItem &old_item,
                                   const domain::Product &product) {
  // Create updated item
  domain::WishlistItem updated_item = old_item;
  if (!old_item.title_locked && !product.title.empty()) {
    updated_item.title = product.title;
  }
  if (product.price > 0.01) {
    updated_item.current_price = product.price;
  } else if (product.in_stock && product.price < 0.01) {
    // If in stock but price is 0, likely a scraper parsing error. Log it.
    Logger::instance().warning(
        fmt::format("Scraped 0 price for in-stock item: {}", product.title));
  }
  updated_item.in_stock = product.in_stock;
  updated_item.is_uhd_4k = product.is_uhd_4k;
  updated_item.image_url = product.image_url;
  if (!product.local_image_path.empty()) {
    updated_item.local_image_path = product.local_image_path;
  } else if (product.image_url != old_item.image_url) {
    // If URL changed but no new local path, clear the old one to avoid mismatch
    // Or we could try to re-download here? For now, let's keep it safe.
    // But typically, if URL changes, we want the new image.
    // If download failed, we might have an empty local path.
    // If we keep the old local path, we show the wrong image.
    // If we clear it, we show the remote URL (fallback).
    // updated_item.local_image_path = "";
    // ACTUALLY, the frontend falls back to image_url if local is missing.
    // So if new URL exists but download failed, we should probably clear local
    // path if the URL differs.
    if (!product.image_url.empty()) {
      updated_item.local_image_path = "";
    }
  }
  updated_item.source = product.source;
  updated_item.last_checked = product.last_updated;

  // Detect changes before updating
  auto changes = change_detector_.detectChanges(old_item, updated_item);

  // Update in database
  if (!repo.update(updated_item)) {
    Logger::instance().error(
        fmt::format("Failed to update wishlist item: {}", updated_item.url));
    return;
  }

  // Record price history
  infrastructure::PriceHistoryRepository history_repo;
  history_repo.addEntry(updated_item.id, updated_item.current_price,
                        updated_item.in_stock);

  // Log changes
  if (!changes.empty()) {
    Logger::instance().info(fmt::format("Detected {} change(s) for: {}",
                                        changes.size(), updated_item.title));
    for (const auto &change : changes) {
      Logger::instance().info(fmt::format("  - {}", change.describe()));
    }
  }
}

} // namespace bluray::application
