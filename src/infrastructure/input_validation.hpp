#pragma once

#include <algorithm>
#include <array>
#include <string_view>

namespace bluray::infrastructure {

/**
 * Whitelists for pagination and filtering parameters to prevent SQL injection
 */
namespace validation {

// Whitelist for sort_by values
constexpr std::array<std::string_view, 3> VALID_SORT_FIELDS = {"price", "title",
                                                                "date"};

// Whitelist for sort_order values
constexpr std::array<std::string_view, 2> VALID_SORT_ORDERS = {"asc", "desc"};

// Whitelist for filter_stock values
constexpr std::array<std::string_view, 2> VALID_STOCK_FILTERS = {
    "in_stock", "out_of_stock"};

/**
 * Validates that a value is in the allowed list
 * @param value The value to validate
 * @param whitelist The array of allowed values
 * @return true if value is in the whitelist, false otherwise
 */
template <size_t N>
inline bool isValidValue(std::string_view value,
                         const std::array<std::string_view, N> &whitelist) {
  return std::any_of(whitelist.begin(), whitelist.end(),
                     [&value](std::string_view allowed) {
                       return value == allowed;
                     });
}

} // namespace validation
} // namespace bluray::infrastructure
