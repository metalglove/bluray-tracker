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

/**
 * Escapes HTML special characters to prevent XSS attacks
 * @param str The string to escape
 * @return Escaped string safe for HTML output
 */
inline std::string escapeHtml(std::string_view str) {
  std::string result;
  result.reserve(str.size() * 1.2); // Reserve extra space for escapes
  
  for (char c : str) {
    switch (c) {
      case '&':  result += "&amp;"; break;
      case '<':  result += "&lt;"; break;
      case '>':  result += "&gt;"; break;
      case '"':  result += "&quot;"; break;
      case '\'': result += "&#39;"; break;
      default:   result += c; break;
    }
  }
  
  return result;
}

/**
 * Escapes JavaScript string content to prevent XSS in onclick handlers
 * @param str The string to escape
 * @return Escaped string safe for JavaScript string literals
 */
inline std::string escapeJs(std::string_view str) {
  std::string result;
  result.reserve(str.size() * 1.2);
  
  for (char c : str) {
    switch (c) {
      case '\\': result += "\\\\"; break;
      case '\'': result += "\\'"; break;
      case '"':  result += "\\\""; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      case '<':  result += "\\x3C"; break;
      case '>':  result += "\\x3E"; break;
      default:   result += c; break;
    }
  }
  
  return result;
}

/**
 * Validates TMDb rating is in valid range
 * @param rating The rating to validate
 * @return true if rating is between 0.0 and 10.0
 */
inline bool isValidTmdbRating(double rating) {
  return rating >= 0.0 && rating <= 10.0;
}

/**
 * Validates IMDb ID format (tt followed by 7-8 digits)
 * @param imdb_id The IMDb ID to validate
 * @return true if format is valid
 */
inline bool isValidImdbId(std::string_view imdb_id) {
  if (imdb_id.empty()) return true; // Allow empty for optional field
  
  if (imdb_id.size() < 9 || imdb_id.size() > 10) return false;
  if (imdb_id[0] != 't' || imdb_id[1] != 't') return false;
  
  for (size_t i = 2; i < imdb_id.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(imdb_id[i]))) {
      return false;
    }
  }
  
  return true;
}

/**
 * Validates YouTube trailer key format (11 characters, alphanumeric with - and _)
 * @param trailer_key The trailer key to validate
 * @return true if format is valid
 */
inline bool isValidTrailerKey(std::string_view trailer_key) {
  if (trailer_key.empty()) return true; // Allow empty for optional field
  
  if (trailer_key.size() != 11) return false;
  
  for (char c : trailer_key) {
    unsigned char uc = static_cast<unsigned char>(c);
    if (!std::isalnum(uc) && c != '-' && c != '_') {
      return false;
    }
  }
  
  return true;
}

/**
 * Validates tag name (non-empty, reasonable length, safe characters)
 * @param name The tag name to validate
 * @param max_length Maximum allowed length (default 50)
 * @return true if name is valid
 */
inline bool isValidTagName(std::string_view name, size_t max_length = 50) {
  if (name.empty() || name.size() > max_length) return false;
  
  // Check for control characters
  for (char c : name) {
    unsigned char uc = static_cast<unsigned char>(c);
    if (uc < 32 || uc == 127) return false;
  }
  
  return true;
}

/**
 * Validates CSS hex color format (#RGB or #RRGGBB or #RRGGBBAA)
 * @param color The color string to validate
 * @return true if format is valid
 */
inline bool isValidHexColor(std::string_view color) {
  if (color.empty() || color[0] != '#') return false;
  
  size_t len = color.size();
  if (len != 4 && len != 7 && len != 9) return false;
  
  for (size_t i = 1; i < len; ++i) {
    char c = color[i];
    unsigned char uc = static_cast<unsigned char>(c);
    if (!std::isxdigit(uc)) return false;
  }
  
  return true;
}

/**
 * Sanitizes CSS color to safe default if invalid
 * @param color The color string to sanitize
 * @param default_color Default color if validation fails
 * @return Valid color string
 */
inline std::string sanitizeColor(const std::string &color, 
                                 std::string_view default_color = "#667eea") {
  return isValidHexColor(color) ? color : std::string(default_color);
}

} // namespace validation
} // namespace bluray::infrastructure
