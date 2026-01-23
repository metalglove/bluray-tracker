#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace bluray::domain {

/**
 * Represents a detected deal on a Blu-ray/UHD product
 */
struct Deal {
  int64_t id{0};
  std::string url;
  std::string title;
  std::string source; // "amazon.nl", "bol.com", etc.

  double original_price{0.0};
  double deal_price{0.0};
  double discount_percentage{0.0};

  std::string deal_type; // "lightning", "daily", "promotion"
  std::optional<std::chrono::system_clock::time_point> ends_at;

  bool is_uhd_4k{false};
  std::string image_url;
  std::string local_image_path;

  std::chrono::system_clock::time_point discovered_at;
  std::chrono::system_clock::time_point last_checked;

  bool is_active{true};

  // Helper methods
  [[nodiscard]] bool isExpired() const {
    if (!ends_at) {
      return false; // No expiration
    }
    return std::chrono::system_clock::now() > *ends_at;
  }

  [[nodiscard]] double calculateSavings() const {
    return original_price - deal_price;
  }

  [[nodiscard]] int getRemainingHours() const {
    if (!ends_at) {
      return -1; // No expiration
    }
    auto now = std::chrono::system_clock::now();
    if (now > *ends_at) {
      return 0; // Expired
    }
    auto duration =
        std::chrono::duration_cast<std::chrono::hours>(*ends_at - now);
    return static_cast<int>(duration.count());
  }
};

} // namespace bluray::domain
