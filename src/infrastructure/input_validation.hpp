#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
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
 * Converts a string to lowercase
 * @param str The string to convert
 * @return Lowercase version of the string
 */
inline std::string toLower(std::string_view str) {
  std::string result;
  result.reserve(str.size());
  std::transform(str.begin(), str.end(), std::back_inserter(result),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

/**
 * Validates that a value is in the allowed list (case-insensitive)
 * @param value The value to validate
 * @param whitelist The array of allowed values (must be lowercase)
 * @return true if value is in the whitelist, false otherwise
 */
template <size_t N>
inline bool isValidValue(std::string_view value,
                         const std::array<std::string_view, N> &whitelist) {
  std::string lower_value = toLower(value);
  return std::any_of(whitelist.begin(), whitelist.end(),
                     [&lower_value](std::string_view allowed) {
                       return lower_value == allowed;
                     });
}

} // namespace validation
} // namespace bluray::infrastructure
