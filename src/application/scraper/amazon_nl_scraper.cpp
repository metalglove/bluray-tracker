#include "amazon_nl_scraper.hpp"
#include "../../infrastructure/logger.hpp"
#include <format>
#include <algorithm>
#include <cctype>
#include <regex>

namespace bluray::application::scraper {

bool AmazonNlScraper::canHandle(std::string_view url) const {
    return url.find("amazon.nl") != std::string_view::npos;
}

std::optional<domain::Product> AmazonNlScraper::scrape(std::string_view url) {
    using namespace infrastructure;

    Logger::instance().info(std::format("Scraping Amazon.nl: {}", url));

    // Fetch HTML
    auto response = client_.get(url);
    if (!response.success) {
        Logger::instance().error(
            std::format("Failed to fetch Amazon.nl page: {} (status: {})",
                       url, response.status_code)
        );
        return std::nullopt;
    }

    // Parse HTML
    auto scraped_data = parseHtml(response.body);
    if (!scraped_data) {
        Logger::instance().error("Failed to parse Amazon.nl HTML");
        return std::nullopt;
    }

    // Build Product
    domain::Product product{
        .url = std::string(url),
        .title = scraped_data->title,
        .price = scraped_data->price,
        .in_stock = scraped_data->in_stock,
        .is_uhd_4k = scraped_data->is_uhd_4k,
        .image_url = scraped_data->image_url,
        .last_updated = std::chrono::system_clock::now(),
        .source = std::string(getSource())
    };

    Logger::instance().info(
        std::format("Successfully scraped: {} (€{:.2f}, stock: {}, UHD: {})",
                   product.title, product.price, product.in_stock, product.is_uhd_4k)
    );

    return product;
}

std::optional<AmazonNlScraper::ScrapedData> AmazonNlScraper::parseHtml(const std::string& html) {
    GumboOutput* output = gumbo_parse(html.c_str());
    if (!output) {
        return std::nullopt;
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

std::optional<std::string> AmazonNlScraper::extractTitle(GumboNode* root) {
    // Try multiple possible title locations
    const char* title_ids[] = {
        "productTitle",
        "title",
        "btAsinTitle"
    };

    for (const char* id : title_ids) {
        if (auto* node = findElementById(root, id)) {
            if (node->type == GUMBO_NODE_ELEMENT) {
                GumboNode* text_node = nullptr;
                if (node->v.element.children.length > 0) {
                    text_node = static_cast<GumboNode*>(node->v.element.children.data[0]);
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
    }

    return std::nullopt;
}

std::optional<double> AmazonNlScraper::extractPrice(GumboNode* root) {
    // Try multiple price selectors
    const char* price_classes[] = {
        "a-price-whole",
        "a-offscreen",
        "a-price"
    };

    for (const char* class_name : price_classes) {
        auto nodes = findElementsByClass(root, class_name);
        for (auto* node : nodes) {
            if (node->type == GUMBO_NODE_ELEMENT) {
                GumboNode* text_node = nullptr;
                if (node->v.element.children.length > 0) {
                    text_node = static_cast<GumboNode*>(node->v.element.children.data[0]);
                    if (text_node && text_node->type == GUMBO_NODE_TEXT) {
                        std::string price_text = text_node->v.text.text;

                        // Parse price (format: "12,34" or "€ 12,34")
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
        }
    }

    return std::nullopt;
}

bool AmazonNlScraper::extractStockStatus(GumboNode* root) {
    // Check for out of stock indicators
    const char* out_of_stock_classes[] = {
        "availability",
        "a-color-price",
        "a-color-error"
    };

    for (const char* class_name : out_of_stock_classes) {
        auto nodes = findElementsByClass(root, class_name);
        for (auto* node : nodes) {
            if (node->type == GUMBO_NODE_ELEMENT) {
                GumboNode* text_node = nullptr;
                if (node->v.element.children.length > 0) {
                    text_node = static_cast<GumboNode*>(node->v.element.children.data[0]);
                    if (text_node && text_node->type == GUMBO_NODE_TEXT) {
                        std::string text = text_node->v.text.text;
                        std::transform(text.begin(), text.end(), text.begin(), ::tolower);

                        if (text.find("niet op voorraad") != std::string::npos ||
                            text.find("momenteel niet beschikbaar") != std::string::npos ||
                            text.find("out of stock") != std::string::npos) {
                            return false;
                        }
                    }
                }
            }
        }
    }

    // If no out of stock indicator found, assume in stock
    return true;
}

bool AmazonNlScraper::extractUhdStatus(GumboNode* root, const std::string& title) {
    // Check title for UHD/4K indicators
    std::string lower_title = title;
    std::transform(lower_title.begin(), lower_title.end(), lower_title.begin(), ::tolower);

    return lower_title.find("4k") != std::string::npos ||
           lower_title.find("uhd") != std::string::npos ||
           lower_title.find("ultra hd") != std::string::npos;
}

std::optional<std::string> AmazonNlScraper::extractImageUrl(GumboNode* root) {
    // Try to find main product image
    if (auto* node = findElementById(root, "landingImage")) {
        if (node->type == GUMBO_NODE_ELEMENT) {
            GumboAttribute* src_attr = gumbo_get_attribute(&node->v.element.attributes, "src");
            if (src_attr && src_attr->value) {
                return std::string(src_attr->value);
            }
        }
    }

    // Try alternative image locations
    const char* image_ids[] = {"imgBlkFront", "main-image"};
    for (const char* id : image_ids) {
        if (auto* node = findElementById(root, id)) {
            if (node->type == GUMBO_NODE_ELEMENT) {
                GumboAttribute* src_attr = gumbo_get_attribute(&node->v.element.attributes, "src");
                if (src_attr && src_attr->value) {
                    return std::string(src_attr->value);
                }
            }
        }
    }

    return std::nullopt;
}

GumboNode* AmazonNlScraper::findElementById(GumboNode* node, const char* id) {
    if (node->type != GUMBO_NODE_ELEMENT) {
        return nullptr;
    }

    GumboAttribute* id_attr = gumbo_get_attribute(&node->v.element.attributes, "id");
    if (id_attr && strcmp(id_attr->value, id) == 0) {
        return node;
    }

    // Recursively search children
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        if (auto* result = findElementById(static_cast<GumboNode*>(children->data[i]), id)) {
            return result;
        }
    }

    return nullptr;
}

GumboNode* AmazonNlScraper::findElementByClass(GumboNode* node, const char* class_name) {
    if (node->type != GUMBO_NODE_ELEMENT) {
        return nullptr;
    }

    GumboAttribute* class_attr = gumbo_get_attribute(&node->v.element.attributes, "class");
    if (class_attr && strstr(class_attr->value, class_name)) {
        return node;
    }

    // Recursively search children
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        if (auto* result = findElementByClass(static_cast<GumboNode*>(children->data[i]), class_name)) {
            return result;
        }
    }

    return nullptr;
}

std::vector<GumboNode*> AmazonNlScraper::findElementsByClass(GumboNode* node, const char* class_name) {
    std::vector<GumboNode*> results;

    if (node->type != GUMBO_NODE_ELEMENT) {
        return results;
    }

    GumboAttribute* class_attr = gumbo_get_attribute(&node->v.element.attributes, "class");
    if (class_attr && strstr(class_attr->value, class_name)) {
        results.push_back(node);
    }

    // Recursively search children
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        auto child_results = findElementsByClass(static_cast<GumboNode*>(children->data[i]), class_name);
        results.insert(results.end(), child_results.begin(), child_results.end());
    }

    return results;
}

} // namespace bluray::application::scraper
