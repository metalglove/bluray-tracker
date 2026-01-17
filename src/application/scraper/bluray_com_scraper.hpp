#pragma once

#include "../../domain/models.hpp"
#include <string_view>
#include <vector>
#include <optional>
#include <gumbo.h>

namespace bluray::application::scraper {

/**
 * Scraper for blu-ray.com release calendar
 *
 * Scrapes upcoming blu-ray releases from:
 * https://www.blu-ray.com/movies/releasedates.php
 */
class BluRayComScraper {
public:
    /**
     * Scrape release calendar for a specific date range
     *
     * @param url The release calendar URL (e.g., https://www.blu-ray.com/movies/releasedates.php)
     * @return Vector of release calendar items
     */
    std::vector<domain::ReleaseCalendarItem> scrapeReleaseCalendar(std::string_view url);

    /**
     * Check if this scraper can handle the given URL
     */
    bool canHandle(std::string_view url) const;

    /**
     * Get source identifier
     */
    std::string_view getSource() const { return "blu-ray.com"; }

private:
    /**
     * Parse the release calendar page HTML
     */
    std::vector<domain::ReleaseCalendarItem> parseReleaseCalendarPage(const std::string& html);

    /**
     * Parse releases from table rows
     */
    void parseTableReleases(GumboNode* node, std::vector<domain::ReleaseCalendarItem>& items);

    /**
     * Parse releases from div containers
     */
    void parseDivReleases(GumboNode* node, std::vector<domain::ReleaseCalendarItem>& items);

    /**
     * Extract release info from a table row
     */
    std::optional<domain::ReleaseCalendarItem> extractReleaseFromTableRow(GumboNode* tr_node);

    /**
     * Extract release info from a div container
     */
    std::optional<domain::ReleaseCalendarItem> extractReleaseFromDiv(GumboNode* div_node);

    /**
     * Extract text content from a node
     */
    std::string extractText(GumboNode* node);

    /**
     * Find first element of specified tag
     */
    GumboNode* findFirstElement(GumboNode* node, GumboTag tag);

    /**
     * Get attribute value from a node
     */
    std::string getAttributeValue(GumboNode* node, const char* attr_name);

    /**
     * Parse a single release item from HTML
     */
    std::optional<domain::ReleaseCalendarItem> parseReleaseItem(
        const std::string& title,
        const std::string& release_date_str,
        const std::string& format,
        const std::string& studio,
        const std::string& image_url,
        const std::string& product_url,
        const std::string& price_str
    );

    /**
     * Detect if format is UHD 4K
     */
    bool isUHD4K(std::string_view format) const;

    /**
     * Parse price from string (e.g., "$19.99" -> 19.99)
     */
    double parsePrice(std::string_view price_str) const;

    /**
     * Parse release date from string
     */
    std::chrono::system_clock::time_point parseReleaseDate(std::string_view date_str) const;
};

} // namespace bluray::application::scraper
