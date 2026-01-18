#pragma once

#include "../../infrastructure/network_client.hpp"
#include "scraper.hpp"
#include <gumbo.h>
#include <nlohmann/json.hpp>

namespace bluray::application::scraper {

/**
 * Scraper implementation for Bol.com
 */
class BolComScraper : public IScraper {
public:
  std::optional<domain::Product> scrape(std::string_view url) override;
  bool canHandle(std::string_view url) const override;
  std::string_view getSource() const override { return "bol.com"; }

private:
  struct ScrapedData {
    std::string title;
    double price{0.0};
    bool in_stock{false};
    bool is_uhd_4k{false};
    std::string image_url;
  };

  std::optional<ScrapedData> parseJsonLd(GumboNode *root,
                                         const std::string &url);
  std::optional<ScrapedData> parseHtml(const std::string &html,
                                       const std::string &url);

  // Helper methods for parsing specific elements
  std::optional<std::string> extractTitle(GumboNode *root);
  std::optional<double> extractPrice(GumboNode *root);
  bool extractStockStatus(GumboNode *root);
  bool extractUhdStatus(GumboNode *root, const std::string &title);
  std::optional<std::string> extractImageUrl(GumboNode *root);
  std::optional<std::string> extractJsonLdScript(GumboNode *root);

  // Recursive search helpers
  GumboNode *findElementByClass(GumboNode *node, const char *class_name);
  std::vector<GumboNode *> findElementsByClass(GumboNode *node,
                                               const char *class_name);
  GumboNode *findElementByTag(GumboNode *node, GumboTag tag);
  std::optional<std::string> extractMetaProperty(GumboNode *node,
                                                 const std::string &property);

  infrastructure::NetworkClient client_;
};

} // namespace bluray::application::scraper
