#pragma once

#include <string>
#include <chrono>
#include <optional>

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
};

/**
 * Item in the release calendar
 */
struct ReleaseCalendarItem {
    int id{0};
    std::string title;
    std::chrono::system_clock::time_point release_date;
    std::string format;           // "Blu-ray", "UHD 4K", "3D Blu-ray", etc.
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

    [[nodiscard]] int offset() const {
        return (page - 1) * page_size;
    }

    [[nodiscard]] int limit() const {
        return page_size;
    }
};

/**
 * Paginated result wrapper
 */
template<typename T>
struct PaginatedResult {
    std::vector<T> items;
    int total_count{0};
    int page{1};
    int page_size{20};

    [[nodiscard]] int total_pages() const {
        return (total_count + page_size - 1) / page_size;
    }

    [[nodiscard]] bool has_next() const {
        return page < total_pages();
    }

    [[nodiscard]] bool has_previous() const {
        return page > 1;
    }
};

} // namespace bluray::domain
