#include "bluray_com_scraper.hpp"
#include "../../infrastructure/logger.hpp"
#include "../../infrastructure/network_client.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fmt/format.h>
#include <gumbo.h>
#include <iomanip>
#include <regex>
#include <sstream>

namespace bluray::application::scraper {

bool BluRayComScraper::canHandle(std::string_view url) const {
  return url.find("blu-ray.com") != std::string_view::npos &&
         url.find("releasedates.php") != std::string_view::npos;
}

std::vector<domain::ReleaseCalendarItem>
BluRayComScraper::scrapeReleaseCalendar(std::string_view url) {
  using namespace infrastructure;

  Logger::instance().info(
      fmt::format("Scraping blu-ray.com release calendar: {}", url));

  // Use NetworkClient with proper headers
  NetworkClient client;

  // Fetch HTML with custom user agent to avoid 403 errors
  auto response = client.get(url);
  if (!response.success) {
    Logger::instance().error(fmt::format(
        "Failed to fetch blu-ray.com release calendar: {} (status: {})", url,
        response.status_code));
    return {};
  }

  // Parse HTML
  auto items = parseReleaseCalendarPage(response.body);

  Logger::instance().info(fmt::format(
      "Successfully scraped {} release calendar items from blu-ray.com",
      items.size()));

  return items;
}

std::vector<domain::ReleaseCalendarItem>
BluRayComScraper::parseReleaseCalendarPage(const std::string &html) {
  std::vector<domain::ReleaseCalendarItem> items;

  GumboOutput *output = gumbo_parse(html.c_str());
  if (!output) {
    infrastructure::Logger::instance().error(
        "Failed to parse blu-ray.com HTML");
    return items;
  }

  // Parse the page structure
  // Blu-ray.com typically uses table rows for releases or div containers
  parseTableReleases(output->root, items);

  // If no items found in tables, try div-based layout
  if (items.empty()) {
    parseDivReleases(output->root, items);
  }

  infrastructure::Logger::instance().info(
      fmt::format("Parsed {} release calendar items from HTML", items.size()));

  gumbo_destroy_output(&kGumboDefaultOptions, output);
  return items;
}

void BluRayComScraper::parseTableReleases(
    GumboNode *node, std::vector<domain::ReleaseCalendarItem> &items) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return;
  }

  GumboElement *element = &node->v.element;

  // Look for table rows
  if (element->tag == GUMBO_TAG_TR) {
    auto item = extractReleaseFromTableRow(node);
    if (item) {
      items.push_back(*item);
    }
  }

  // Recursively process children
  GumboVector *children = &element->children;
  for (unsigned int i = 0; i < children->length; ++i) {
    parseTableReleases(static_cast<GumboNode *>(children->data[i]), items);
  }
}

void BluRayComScraper::parseDivReleases(
    GumboNode *node, std::vector<domain::ReleaseCalendarItem> &items) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return;
  }

  GumboElement *element = &node->v.element;

  // Look for div containers that might contain release info
  if (element->tag == GUMBO_TAG_DIV) {
    auto item = extractReleaseFromDiv(node);
    if (item) {
      items.push_back(*item);
    }
  }

  // Recursively process children
  GumboVector *children = &element->children;
  for (unsigned int i = 0; i < children->length; ++i) {
    parseDivReleases(static_cast<GumboNode *>(children->data[i]), items);
  }
}

std::optional<domain::ReleaseCalendarItem>
BluRayComScraper::extractReleaseFromTableRow(GumboNode *tr_node) {
  if (tr_node->type != GUMBO_NODE_ELEMENT ||
      tr_node->v.element.tag != GUMBO_TAG_TR) {
    return std::nullopt;
  }

  std::string release_date_str, title, format, studio, image_url, product_url,
      price_str;

  // Extract data from table cells (td elements)
  GumboVector *children = &tr_node->v.element.children;
  std::vector<std::string> cell_texts;

  for (unsigned int i = 0; i < children->length; ++i) {
    GumboNode *cell = static_cast<GumboNode *>(children->data[i]);
    if (cell->type == GUMBO_NODE_ELEMENT &&
        cell->v.element.tag == GUMBO_TAG_TD) {
      std::string text = extractText(cell);
      cell_texts.push_back(text);

      // Look for links and images in cells
      auto link = findFirstElement(cell, GUMBO_TAG_A);
      if (link && product_url.empty()) {
        product_url = getAttributeValue(link, "href");
      }

      auto img = findFirstElement(cell, GUMBO_TAG_IMG);
      if (img && image_url.empty()) {
        image_url = getAttributeValue(img, "src");
      }
    }
  }

  // Typical column structure: [Date, Cover Image, Title, Format, Studio, Price]
  // Adjust indices based on actual site structure
  if (cell_texts.size() >= 3) {
    // Flexible parsing - look for date-like patterns
    for (const auto &text : cell_texts) {
      if (!text.empty()) {
        // Try to identify what each field is
        if (release_date_str.empty() &&
            (text.find("20") != std::string::npos ||
             text.find("Jan") != std::string::npos ||
             text.find("Feb") != std::string::npos)) {
          release_date_str = text;
        } else if (title.empty() && text.length() > 10 &&
                   product_url.empty() == false) {
          title = text;
        } else if (format.empty() &&
                   (text.find("Blu-ray") != std::string::npos ||
                    text.find("4K") != std::string::npos ||
                    text.find("UHD") != std::string::npos)) {
          format = text;
        } else if (price_str.empty() && (text.find("$") != std::string::npos ||
                                         text.find("€") != std::string::npos ||
                                         std::isdigit(text[0]))) {
          price_str = text;
        }
      }
    }

    // Only create item if we have at least title and date
    if (!title.empty() && !release_date_str.empty()) {
      if (format.empty())
        format = "Blu-ray";
      return parseReleaseItem(title, release_date_str, format, studio,
                              image_url, product_url, price_str);
    }
  }

  return std::nullopt;
}

std::optional<domain::ReleaseCalendarItem>
BluRayComScraper::extractReleaseFromDiv(GumboNode *div_node) {
  // Similar extraction logic for div-based layouts
  // This is a simplified version - would need to be adapted based on actual
  // HTML structure
  if (div_node->type != GUMBO_NODE_ELEMENT) {
    return std::nullopt;
  }

  std::string title = extractText(div_node);
  if (title.length() < 5) {
    return std::nullopt; // Too short to be a real title
  }

  // Look for links and dates within the div
  auto link = findFirstElement(div_node, GUMBO_TAG_A);
  std::string product_url = link ? getAttributeValue(link, "href") : "";

  // For now, return nullopt as div parsing needs more specific implementation
  return std::nullopt;
}

std::string BluRayComScraper::extractText(GumboNode *node) {
  if (node->type == GUMBO_NODE_TEXT) {
    return std::string(node->v.text.text);
  }
  if (node->type != GUMBO_NODE_ELEMENT) {
    return "";
  }

  std::string text;
  GumboVector *children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    text += extractText(static_cast<GumboNode *>(children->data[i]));
  }

  // Trim whitespace
  auto start = text.find_first_not_of(" \n\r\t");
  if (start == std::string::npos) {
    // String is empty or all whitespace
    text.clear();
    return text;
  }

  text.erase(0, start);

  auto end = text.find_last_not_of(" \n\r\t");
  if (end != std::string::npos) {
    text.erase(end + 1);
  }
  return text;
}

GumboNode *BluRayComScraper::findFirstElement(GumboNode *node, GumboTag tag) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return nullptr;
  }

  if (node->v.element.tag == tag) {
    return node;
  }

  GumboVector *children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    GumboNode *result =
        findFirstElement(static_cast<GumboNode *>(children->data[i]), tag);
    if (result) {
      return result;
    }
  }

  return nullptr;
}

std::string BluRayComScraper::getAttributeValue(GumboNode *node,
                                                const char *attr_name) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return "";
  }

  GumboVector *attributes = &node->v.element.attributes;
  for (unsigned int i = 0; i < attributes->length; ++i) {
    GumboAttribute *attr = static_cast<GumboAttribute *>(attributes->data[i]);
    if (std::strcmp(attr->name, attr_name) == 0) {
      return std::string(attr->value);
    }
  }

  return "";
}

std::optional<domain::ReleaseCalendarItem> BluRayComScraper::parseReleaseItem(
    const std::string &title, const std::string &release_date_str,
    const std::string &format, const std::string &studio,
    const std::string &image_url, const std::string &product_url,
    const std::string &price_str) {
  domain::ReleaseCalendarItem item;

  try {
    item.title = title;
    item.release_date = parseReleaseDate(release_date_str);
    item.format = format;
    item.studio = studio;
    item.image_url = image_url;
    item.product_url = product_url;
    item.is_uhd_4k = isUHD4K(format);
    item.price = parsePrice(price_str);

    // Determine if it's a preorder based on release date
    auto now = std::chrono::system_clock::now();
    item.is_preorder = item.release_date > now;

    item.created_at = now;
    item.last_updated = now;

    return item;
  } catch (const std::exception &e) {
    infrastructure::Logger::instance().error(
        fmt::format("Failed to parse release item: {}", e.what()));
    return std::nullopt;
  }
}

bool BluRayComScraper::isUHD4K(std::string_view format) const {
  std::string format_lower;
  format_lower.resize(format.size());
  std::transform(format.begin(), format.end(), format_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  return format_lower.find("uhd") != std::string::npos ||
         format_lower.find("4k") != std::string::npos ||
         format_lower.find("ultra hd") != std::string::npos;
}

double BluRayComScraper::parsePrice(std::string_view price_str) const {
  if (price_str.empty()) {
    return 0.0;
  }

  // Remove currency symbols and whitespace
  std::string cleaned;
  for (char c : price_str) {
    if (std::isdigit(c) || c == '.' || c == ',') {
      cleaned += c;
    }
  }

  // Replace comma with period for decimal parsing
  std::replace(cleaned.begin(), cleaned.end(), ',', '.');

  try {
    return std::stod(cleaned);
  } catch (...) {
    return 0.0;
  }
}

std::chrono::system_clock::time_point
BluRayComScraper::parseReleaseDate(std::string_view date_str) const {
  // Common date formats on blu-ray.com:
  // "Jan 15, 2024"
  // "2024-01-15"
  // "15 Jan 2024"

  std::tm tm = {};
  std::string date_str_copy{date_str};

  // Try different formats
  const char *formats[] = {
      "%b %d, %Y", // Jan 15, 2024
      "%Y-%m-%d",  // 2024-01-15
      "%d %b %Y",  // 15 Jan 2024
      "%m/%d/%Y"   // 01/15/2024
  };

  for (const char *format : formats) {
    std::istringstream iss{date_str_copy};
    iss >> std::get_time(&tm, format);

    if (!iss.fail()) {
      auto time_t = std::mktime(&tm);
      return std::chrono::system_clock::from_time_t(time_t);
    }
  }

  // If parsing fails, return current time
  infrastructure::Logger::instance().warning(fmt::format(
      "Failed to parse release date: {}, using current time", date_str));
  return std::chrono::system_clock::now();
}

} // namespace bluray::application::scraper
