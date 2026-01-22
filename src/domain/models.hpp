#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace bluray::domain {

/**
 * Core product information scraped from websites
 */
struct Product {
  std::string url;
  std::string title;
  double price{0.0};
  bool in_stock{false};
  bool is_uhd_4k{false};
  std::string image_url;
  std::string local_image_path;
  std::chrono::system_clock::time_point last_updated;

  // Source website (amazon.nl or bol.com)
  std::string source;
};

/**
 * Item on the user's wishlist with desired price threshold
 */
struct WishlistItem {
  int id{0};
  std::string url;
  std::string title;
  double current_price{0.0};
  double desired_max_price{0.0};
  bool in_stock{false};
  bool is_uhd_4k{false};
  std::string image_url;
  std::string local_image_path;
  std::string source;
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point last_checked;

  // Notification preferences
  bool notify_on_price_drop{true};
  bool notify_on_stock{true};

  // Scraper override protection
  bool title_locked{false};

  // TMDb/IMDb integration
  int tmdb_id{0};
  std::string imdb_id;
  double tmdb_rating{0.0};
  std::string trailer_key; // YouTube video key

  // Edition & bonus features
  std::string edition_type;      // "Standard", "Steelbook", "Collector's", etc.
  bool has_slipcover{false};
  bool has_digital_copy{false};
  std::string bonus_features;    // JSON array of bonus features
};

/**
 * Item in the user's collection
 */
struct CollectionItem {
  int id{0};
  std::string url;
  std::string title;
  double purchase_price{0.0};
  bool is_uhd_4k{false};
  std::string image_url;
  std::string local_image_path;
  std::string source;
  std::chrono::system_clock::time_point purchased_at;
  std::chrono::system_clock::time_point added_at;

  // Optional notes
  std::string notes;

  // TMDb/IMDb integration
  int tmdb_id{0};
  std::string imdb_id;
  double tmdb_rating{0.0};
  std::string trailer_key; // YouTube video key

  // Edition & bonus features
  std::string edition_type;      // "Standard", "Steelbook", "Collector's", etc.
  bool has_slipcover{false};
  bool has_digital_copy{false};
  std::string bonus_features;    // JSON array of bonus features
};

/**
 * Item in the release calendar
 */
struct ReleaseCalendarItem {
  int id{0};
  std::string title;
  std::chrono::system_clock::time_point release_date;
  std::string format; // "Blu-ray", "UHD 4K", "3D Blu-ray", etc.
  std::string studio;
  std::string image_url;
  std::string local_image_path;
  std::string product_url;
  bool is_uhd_4k{false};
  bool is_preorder{false};
  double price{0.0};
  std::string notes;
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point last_updated;
};

/**
 * User-defined tag for organizing items
 */
struct Tag {
  int id{0};
  std::string name;
  std::string color{"#667eea"}; // Default purple
};

/**
 * Mapping between items and tags
 */
struct ItemTag {
  int item_id{0};
  std::string item_type; // "wishlist" or "collection"
  int tag_id{0};
};

/**
 * Types of changes that can be detected
 */
enum class ChangeType {
  PriceDroppedBelowThreshold,
  BackInStock,
  PriceChanged,
  OutOfStock
};

/**
 * Event representing a detected change
 */
struct ChangeEvent {
  ChangeType type;
  WishlistItem item;

  // Additional context
  std::optional<double> old_price;
  std::optional<double> new_price;
  std::optional<bool> old_stock_status;
  std::optional<bool> new_stock_status;

  std::chrono::system_clock::time_point detected_at;

  /**
   * Generate human-readable description of the change
   */
  [[nodiscard]] std::string describe() const;
};

/**
 * Pagination parameters for queries
 */
struct PaginationParams {
  int page{1};
  int page_size{20};
  std::string sort_by;       // "price", "date", "title"
  std::string sort_order;    // "asc", "desc"
  std::string filter_stock;  // "in_stock", "out_of_stock"
  std::string filter_source; // "amazon.nl", "bol.com"
  std::string search_query;

  [[nodiscard]] int offset() const { return (page - 1) * page_size; }

  [[nodiscard]] int limit() const { return page_size; }
};

/**
 * Paginated result wrapper
 */
template <typename T> struct PaginatedResult {
  std::vector<T> items;
  int total_count{0};
  int page{1};
  int page_size{20};
  double total_value{0.0};

  [[nodiscard]] int total_pages() const {
    if (page_size == 0) {
      return 1;
    }
    return (total_count + page_size - 1) / page_size;
  }

  [[nodiscard]] bool has_next() const { return page < total_pages(); }

  [[nodiscard]] bool has_previous() const { return page > 1; }
};

} // namespace bluray::domain
