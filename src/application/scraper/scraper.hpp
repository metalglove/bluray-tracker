#pragma once

#include "../../domain/models.hpp"
#include <string_view>
#include <optional>
#include <memory>

namespace bluray::application::scraper {

/**
 * Abstract scraper interface (Factory pattern)
 */
class IScraper {
public:
    virtual ~IScraper() = default;

    /**
     * Scrape product information from URL
     */
    virtual std::optional<domain::Product> scrape(std::string_view url) = 0;

    /**
     * Check if this scraper can handle the given URL
     */
    virtual bool canHandle(std::string_view url) const = 0;

    /**
     * Get source identifier
     */
    virtual std::string_view getSource() const = 0;
};

/**
 * Factory for creating appropriate scraper based on URL
 */
class ScraperFactory {
public:
    /**
     * Create scraper for URL, returns nullptr if no scraper can handle it
     */
    static std::unique_ptr<IScraper> create(std::string_view url);

    /**
     * Get all available scrapers
     */
    static std::vector<std::unique_ptr<IScraper>> createAll();
};

} // namespace bluray::application::scraper
