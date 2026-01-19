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
 * Sanitizes a string for safe logging (removes newlines, limits length)
 * @param str The string to sanitize
 * @param max_length Maximum length of the output string (default 100)
 * @return Sanitized string safe for logging
 */
inline std::string sanitizeForLog(std::string_view str,
                                  size_t max_length = 100) {
  std::string result;
  result.reserve(std::min(str.size(), max_length));

  for (size_t i = 0; i < str.size() && result.size() < max_length; ++i) {
    char c = str[i];
    // Replace newlines, carriage returns, and other control characters with
    // space
    unsigned char uc = static_cast<unsigned char>(c);
    if (c == '\n' || c == '\r' || c == '\t' || uc < 32) {
      result += ' ';
    } else {
      result += c;
    }
  }

  if (str.size() > max_length) {
    result += "...";
  }

  return result;
}

/**
 * Validates that a value is in the allowed list (case-insensitive)
 * @param value The value to validate
 * @param whitelist The array of allowed values (lowercase)
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

/**
 * Validates that a value is in the allowed list and returns normalized value
 * @param value The value to validate
 * @param whitelist The array of allowed values (lowercase)
 * @param normalized_value Output parameter for the normalized (lowercase) value
 * @return true if value is in the whitelist, false otherwise
 */
template <size_t N>
inline bool
isValidValueNormalized(std::string_view value,
                       const std::array<std::string_view, N> &whitelist,
                       std::string &normalized_value) {
  normalized_value = toLower(value);
  return std::any_of(whitelist.begin(), whitelist.end(),
                     [&normalized_value](std::string_view allowed) {
                       return normalized_value == allowed;
                     });
}

} // namespace validation
} // namespace bluray::infrastructure
