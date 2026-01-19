#include "bol_com_scraper.hpp"
#include "../../infrastructure/logger.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fmt/format.h>
#include <regex>

namespace bluray::application::scraper {

bool BolComScraper::canHandle(std::string_view url) const {
  return url.find("bol.com") != std::string_view::npos;
}

std::optional<domain::Product> BolComScraper::scrape(std::string_view url) {
  using namespace infrastructure;

  Logger::instance().info(fmt::format("Scraping Bol.com: {}", url));

  // Fetch HTML
  auto response = client_.get(url);
  if (!response.success) {
    Logger::instance().error(
        fmt::format("Failed to fetch Bol.com page: {} (status: {})", url,
                    response.status_code));
    return std::nullopt;
  }

  // Parse HTML
  auto scraped_data = parseHtml(response.body, std::string(url));
  if (!scraped_data) {
    Logger::instance().error("Failed to parse Bol.com HTML");
    return std::nullopt;
  }

  // Build Product
  domain::Product product{.url = std::string(url),
                          .title = scraped_data->title,
                          .price = scraped_data->price,
                          .in_stock = scraped_data->in_stock,
                          .is_uhd_4k = scraped_data->is_uhd_4k,
                          .image_url = scraped_data->image_url,
                          .last_updated = std::chrono::system_clock::now(),
                          .source = std::string(getSource())};

  Logger::instance().info(fmt::format(
      "Successfully scraped: {} (€{:.2f}, stock: {}, UHD: {})", product.title,
      product.price, product.in_stock, product.is_uhd_4k));

  return product;
}

std::optional<BolComScraper::ScrapedData>
BolComScraper::parseHtml(const std::string &html, const std::string &url) {
  GumboOutput *output = gumbo_parse(html.c_str());
  if (!output) {
    return std::nullopt;
  }

  // Try JSON-LD first (most reliable)
  if (auto json_data = parseJsonLd(output->root, url)) {
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return json_data;
  }

  ScrapedData data;

  // Extract title
  if (auto title = extractTitle(output->root)) {
    data.title = *title;
  } else {
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return std::nullopt;
  }

  // Extract price
  if (auto price = extractPrice(output->root)) {
    data.price = *price;
  }

  // Extract stock status
  data.in_stock = extractStockStatus(output->root);

  // Extract UHD/4K status from title
  data.is_uhd_4k = extractUhdStatus(output->root, data.title);

  // Extract image URL
  if (auto image_url = extractImageUrl(output->root)) {
    data.image_url = *image_url;
  }

  gumbo_destroy_output(&kGumboDefaultOptions, output);
  return data;
}

std::optional<BolComScraper::ScrapedData>
BolComScraper::parseJsonLd(GumboNode *root, const std::string &url) {
  auto script_content = extractJsonLdScript(root);
  if (!script_content) {
    return std::nullopt;
  }

  try {
    auto j = nlohmann::json::parse(*script_content);

    // Find the correct product/movie item
    // It could be a single object or a @graph array
    const nlohmann::json *item = &j;

    // Helper to check if item matches our URL
    // Helper to extract Bol.com ID from a URL
    auto extractId = [](std::string_view url_str) -> std::string {
      // Look for 13+ digits pattern which is typical for Bol.com IDs
      // Identifiers usually appear after /p/title/ or at end of path
      std::regex id_regex(R"((\d{13,}))");
      std::smatch match;
      std::string url_string(url_str);
      if (std::regex_search(url_string, match, id_regex)) {
        return match[1].str();
      }
      return "";
    };

    std::string target_id = extractId(url);
    if (target_id.empty()) {
      // Fallback to simplistic slash logic if regex fails (though unlikely for
      // valid bol/p/ urls) Kept for backward compat with non-standard URLs
    } else {
      infrastructure::Logger::instance().debug(
          fmt::format("Bol.com Target ID: {}", target_id));
    }

    // Helper to check if item matches our URL
    auto matchesUrl = [&](const nlohmann::json &obj) {
      if (!obj.contains("url"))
        return false;
      std::string obj_url = obj["url"].get<std::string>();

      // Strict ID match if we found one
      if (!target_id.empty()) {
        std::string obj_id = extractId(obj_url);
        if (!obj_id.empty()) {
          return obj_id == target_id;
        }
        // If obj has no ID in URL (weird), fallback to substring
        return obj_url.find(target_id) != std::string_view::npos;
      }

      // Fallback to simple substring match
      return obj_url.find(url) != std::string::npos;
    };

    // If it's a Movie/Product with workExample (variants)
    if (item->contains("workExample") && (*item)["workExample"].is_array()) {
      bool found_variant = false;
      for (const auto &variant : (*item)["workExample"]) {
        if (matchesUrl(variant)) {
          item = &variant;
          found_variant = true;
          infrastructure::Logger::instance().debug(
              "Found matching variant in JSON-LD");
          break;
        }
      }
      if (!found_variant && !target_id.empty()) {
        infrastructure::Logger::instance().warning(
            fmt::format("No variant matched Target ID: {}", target_id));
      }
    }

    ScrapedData data;

    if (item->contains("name")) {
      data.title = (*item)["name"].get<std::string>();
    } else if (j.contains("name")) {
      data.title = j["name"].get<std::string>();
    }

    // Default image from main object if variant lacks it
    if (item->contains("image")) {
      auto &img = (*item)["image"];
      if (img.is_object() && img.contains("url")) {
        data.image_url = img["url"].get<std::string>();
      } else if (img.is_string()) {
        data.image_url = img.get<std::string>();
      }
    }
    if (data.image_url.empty() && j.contains("image")) {
      auto &img = j["image"];
      if (img.is_object() && img.contains("url")) {
        data.image_url = img["url"].get<std::string>();
      }
    }

    // UHD Check
    std::string lower_title = data.title;
    std::transform(lower_title.begin(), lower_title.end(), lower_title.begin(),
                   ::tolower);
    data.is_uhd_4k = lower_title.find("4k") != std::string::npos ||
                     lower_title.find("uhd") != std::string::npos;

    // Price and Stock
    if (item->contains("offers")) {
      const auto &offers = (*item)["offers"];
      // Could be a single offer or array? Usually single object for specific
      // variant
      const nlohmann::json *offer = nullptr;

      if (offers.is_array() && !offers.empty()) {
        offer = &offers[0]; // Take first offer?
      } else if (offers.is_object()) {
        offer = &offers;
      }

      if (offer) {
        if (offer->contains("price")) {
          std::string price_str = (*offer)["price"].get<std::string>();
          try {
            data.price = std::stod(price_str);
          } catch (...) {
          }
        }
        if (offer->contains("availability")) {
          std::string avail = (*offer)["availability"].get<std::string>();
          data.in_stock = (avail.find("InStock") != std::string::npos);
        }
      }
    }

    if (!data.title.empty()) {
      infrastructure::Logger::instance().info(
          fmt::format("Parsed JSON-LD: Title='{}', Price={:.2f}, Stock={}",
                      data.title, data.price, data.in_stock));
      return data;
    }

  } catch (const std::exception &e) {
    infrastructure::Logger::instance().warning(
        fmt::format("JSON-LD parsing failed: {}", e.what()));
  }

  return std::nullopt;
}

std::optional<std::string> BolComScraper::extractJsonLdScript(GumboNode *root) {
  if (root->type != GUMBO_NODE_ELEMENT)
    return std::nullopt;

  if (root->v.element.tag == GUMBO_TAG_SCRIPT) {
    GumboAttribute *type_attr =
        gumbo_get_attribute(&root->v.element.attributes, "type");
    if (type_attr && std::string(type_attr->value) == "application/ld+json") {
      if (root->v.element.children.length > 0) {
        GumboNode *text =
            static_cast<GumboNode *>(root->v.element.children.data[0]);
        if (text->type == GUMBO_NODE_TEXT) {
          return std::string(text->v.text.text);
        }
      }
    }
  }

  GumboVector *children = &root->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    if (auto result =
            extractJsonLdScript(static_cast<GumboNode *>(children->data[i]))) {
      return result;
    }
  }
  return std::nullopt;
}

std::optional<std::string> BolComScraper::extractTitle(GumboNode *root) {
  // Try og:title meta tag first
  if (auto meta_title = extractMetaProperty(root, "og:title")) {
    return *meta_title;
  }

  // Try h1 tag first (common for product titles)
  if (auto *h1 = findElementByTag(root, GUMBO_TAG_H1)) {
    if (h1->type == GUMBO_NODE_ELEMENT) {
      GumboNode *text_node = nullptr;
      if (h1->v.element.children.length > 0) {
        text_node = static_cast<GumboNode *>(h1->v.element.children.data[0]);
        if (text_node && text_node->type == GUMBO_NODE_TEXT) {
          std::string title = text_node->v.text.text;
          // Trim whitespace
          title.erase(0, title.find_first_not_of(" \t\n\r"));
          title.erase(title.find_last_not_of(" \t\n\r") + 1);
          if (!title.empty()) {
            return title;
          }
        }
      }
    }
  }

  // Try common class names
  const char *title_classes[] = {"product-title", "page-heading", "h1"};

  for (const char *class_name : title_classes) {
    if (auto *node = findElementByClass(root, class_name)) {
      if (node->type == GUMBO_NODE_ELEMENT) {
        GumboNode *text_node = nullptr;
        if (node->v.element.children.length > 0) {
          text_node =
              static_cast<GumboNode *>(node->v.element.children.data[0]);
          if (text_node && text_node->type == GUMBO_NODE_TEXT) {
            std::string title = text_node->v.text.text;
            title.erase(0, title.find_first_not_of(" \t\n\r"));
            title.erase(title.find_last_not_of(" \t\n\r") + 1);
            if (!title.empty()) {
              return title;
            }
          }
        }
      }
    }
  }

  return std::nullopt;
}

std::optional<double> BolComScraper::extractPrice(GumboNode *root) {
  // Try multiple price selectors
  const char *price_classes[] = {"promo-price", "price", "product-price",
                                 "buy-block-price"};

  for (const char *class_name : price_classes) {
    auto nodes = findElementsByClass(root, class_name);
    for (auto *node : nodes) {
      if (node->type == GUMBO_NODE_ELEMENT) {
        // Search for text in node and children
        std::function<std::string(GumboNode *)> getText =
            [&](GumboNode *n) -> std::string {
          if (n->type == GUMBO_NODE_TEXT) {
            return n->v.text.text;
          }
          std::string text;
          if (n->type == GUMBO_NODE_ELEMENT) {
            GumboVector *children = &n->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
              text += getText(static_cast<GumboNode *>(children->data[i]));
            }
          }
          return text;
        };

        std::string price_text = getText(node);

        // Parse price (format: "12,34" or "€ 12,34" or "12.34")
        std::regex price_regex(R"((\d+)[,.](\d+))");
        std::smatch match;
        if (std::regex_search(price_text, match, price_regex)) {
          try {
            double euros = std::stod(match[1].str());
            double cents = std::stod(match[2].str()) / 100.0;
            return euros + cents;
          } catch (...) {
            continue;
          }
        }
      }
    }
  }

  return std::nullopt;
}

bool BolComScraper::extractStockStatus(GumboNode *root) {
  // Check for out of stock indicators
  const char *stock_classes[] = {"availability-block", "buy-block",
                                 "stock-status"};

  for (const char *class_name : stock_classes) {
    auto nodes = findElementsByClass(root, class_name);
    for (auto *node : nodes) {
      std::function<std::string(GumboNode *)> getText =
          [&](GumboNode *n) -> std::string {
        if (n->type == GUMBO_NODE_TEXT) {
          return n->v.text.text;
        }
        std::string text;
        if (n->type == GUMBO_NODE_ELEMENT) {
          GumboVector *children = &n->v.element.children;
          for (unsigned int i = 0; i < children->length; ++i) {
            text += getText(static_cast<GumboNode *>(children->data[i])) + " ";
          }
        }
        return text;
      };

      std::string text = getText(node);
      std::transform(text.begin(), text.end(), text.begin(), ::tolower);

      if (text.find("niet op voorraad") != std::string::npos ||
          text.find("tijdelijk uitverkocht") != std::string::npos ||
          text.find("momenteel niet verkrijgbaar") != std::string::npos ||
          text.find("out of stock") != std::string::npos) {
        return false;
      }
    }
  }

  // If no out of stock indicator found, assume in stock
  return true;
}

bool BolComScraper::extractUhdStatus(GumboNode *root,
                                     const std::string &title) {
  // Check title for UHD/4K indicators
  std::string lower_title = title;
  std::transform(lower_title.begin(), lower_title.end(), lower_title.begin(),
                 ::tolower);

  return lower_title.find("4k") != std::string::npos ||
         lower_title.find("uhd") != std::string::npos ||
         lower_title.find("ultra hd") != std::string::npos ||
         lower_title.find("ultra-hd") != std::string::npos;
}

std::optional<std::string> BolComScraper::extractImageUrl(GumboNode *root) {
  // Try og:image meta tag first (most reliable)
  if (auto meta_image = extractMetaProperty(root, "og:image")) {
    return *meta_image;
  }

  // Find main product image
  const char *image_classes[] = {"product-image", "js_selected_image",
                                 "main-image"};

  for (const char *class_name : image_classes) {
    if (auto *node = findElementByClass(root, class_name)) {
      if (node->type == GUMBO_NODE_ELEMENT &&
          node->v.element.tag == GUMBO_TAG_IMG) {
        GumboAttribute *src_attr =
            gumbo_get_attribute(&node->v.element.attributes, "src");
        if (src_attr && src_attr->value) {
          return std::string(src_attr->value);
        }
      }
      // Check if it's a container with an img child
      if (node->type == GUMBO_NODE_ELEMENT) {
        if (auto *img = findElementByTag(node, GUMBO_TAG_IMG)) {
          GumboAttribute *src_attr =
              gumbo_get_attribute(&img->v.element.attributes, "src");
          if (src_attr && src_attr->value) {
            return std::string(src_attr->value);
          }
        }
      }
    }
  }

  return std::nullopt;
}

std::optional<std::string>
BolComScraper::extractMetaProperty(GumboNode *node,
                                   const std::string &property) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return std::nullopt;
  }

  if (node->v.element.tag == GUMBO_TAG_META) {
    GumboAttribute *prop_attr =
        gumbo_get_attribute(&node->v.element.attributes, "property");
    if (prop_attr && prop_attr->value &&
        std::string(prop_attr->value) == property) {
      GumboAttribute *content_attr =
          gumbo_get_attribute(&node->v.element.attributes, "content");
      if (content_attr && content_attr->value) {
        return std::string(content_attr->value);
      }
    }
  }

  // Recursively search children
  GumboVector *children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    if (auto result = extractMetaProperty(
            static_cast<GumboNode *>(children->data[i]), property)) {
      return result;
    }
  }

  return std::nullopt;
}

GumboNode *BolComScraper::findElementByClass(GumboNode *node,
                                             const char *class_name) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return nullptr;
  }

  GumboAttribute *class_attr =
      gumbo_get_attribute(&node->v.element.attributes, "class");
  if (class_attr && strstr(class_attr->value, class_name)) {
    return node;
  }

  // Recursively search children
  GumboVector *children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    if (auto *result = findElementByClass(
            static_cast<GumboNode *>(children->data[i]), class_name)) {
      return result;
    }
  }

  return nullptr;
}

std::vector<GumboNode *>
BolComScraper::findElementsByClass(GumboNode *node, const char *class_name) {
  std::vector<GumboNode *> results;

  if (node->type != GUMBO_NODE_ELEMENT) {
    return results;
  }

  GumboAttribute *class_attr =
      gumbo_get_attribute(&node->v.element.attributes, "class");
  if (class_attr && strstr(class_attr->value, class_name)) {
    results.push_back(node);
  }

  // Recursively search children
  GumboVector *children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    auto child_results = findElementsByClass(
        static_cast<GumboNode *>(children->data[i]), class_name);
    results.insert(results.end(), child_results.begin(), child_results.end());
  }

  return results;
}

GumboNode *BolComScraper::findElementByTag(GumboNode *node, GumboTag tag) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return nullptr;
  }

  if (node->v.element.tag == tag) {
    return node;
  }

  // Recursively search children
  GumboVector *children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    if (auto *result = findElementByTag(
            static_cast<GumboNode *>(children->data[i]), tag)) {
      return result;
    }
  }

  return nullptr;
}

} // namespace bluray::application::scraper
