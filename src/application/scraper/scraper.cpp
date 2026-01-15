#include "scraper.hpp"
#include "amazon_nl_scraper.hpp"
#include "bol_com_scraper.hpp"

namespace bluray::application::scraper {

std::unique_ptr<IScraper> ScraperFactory::create(std::string_view url) {
    // Try Amazon.nl
    auto amazon = std::make_unique<AmazonNlScraper>();
    if (amazon->canHandle(url)) {
        return amazon;
    }

    // Try Bol.com
    auto bol = std::make_unique<BolComScraper>();
    if (bol->canHandle(url)) {
        return bol;
    }

    // No scraper found
    return nullptr;
}

std::vector<std::unique_ptr<IScraper>> ScraperFactory::createAll() {
    std::vector<std::unique_ptr<IScraper>> scrapers;
    scrapers.push_back(std::make_unique<AmazonNlScraper>());
    scrapers.push_back(std::make_unique<BolComScraper>());
    return scrapers;
}

} // namespace bluray::application::scraper
