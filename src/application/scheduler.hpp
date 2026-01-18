#pragma once

#include "../domain/change_detector.hpp"
#include "../domain/models.hpp"
#include "../infrastructure/image_cache.hpp"
#include "../infrastructure/repositories/wishlist_repository.hpp"
#include "notifier/notifier.hpp"
#include <atomic>
#include <memory>

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

  struct ScrapeProgress {
    int processed;
    int total;
    bool is_active;
  };

  /**
   * Get current scrape progress
   */
  ScrapeProgress getScrapeProgress() const;

private:
  struct ScrapeResult {
    bool success{false};
    domain::Product product;
    std::string error_message;
  };

  int delay_seconds_{8};
  std::atomic<bool> is_running_{false};
  std::atomic<int> scrape_total_{0};
  std::atomic<int> scrape_processed_{0};

  ScrapeResult scrapeProduct(const std::string &url);
  void updateWishlistItem(
      infrastructure::repositories::SqliteWishlistRepository &repo,
      const domain::WishlistItem &old_item, const domain::Product &product);

  domain::ChangeDetector change_detector_;
  std::unique_ptr<infrastructure::ImageCache> image_cache_;
};

} // namespace bluray::application
