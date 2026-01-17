#include "bluray_com_scraper.hpp"
#include "../../infrastructure/logger.hpp"
#include "../../infrastructure/network_client.hpp"
#include <format>
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <iomanip>
#include <gumbo.h>

namespace bluray::application::scraper {

bool BluRayComScraper::canHandle(std::string_view url) const {
    return url.find("blu-ray.com") != std::string_view::npos &&
           url.find("releasedates.php") != std::string_view::npos;
}

std::vector<domain::ReleaseCalendarItem> BluRayComScraper::scrapeReleaseCalendar(std::string_view url) {
    using namespace infrastructure;

    Logger::instance().info(std::format("Scraping blu-ray.com release calendar: {}", url));

    // Use NetworkClient with proper headers
    NetworkClient client;

    // Fetch HTML with custom user agent to avoid 403 errors
    auto response = client.get(url);
    if (!response.success) {
        Logger::instance().error(
            std::format("Failed to fetch blu-ray.com release calendar: {} (status: {})",
                       url, response.status_code)
        );
        return {};
    }

    // Parse HTML
    auto items = parseReleaseCalendarPage(response.body);

    Logger::instance().info(
        std::format("Successfully scraped {} release calendar items from blu-ray.com", items.size())
    );

    return items;
}

std::vector<domain::ReleaseCalendarItem> BluRayComScraper::parseReleaseCalendarPage(const std::string& html) {
    std::vector<domain::ReleaseCalendarItem> items;

    GumboOutput* output = gumbo_parse(html.c_str());
    if (!output) {
        infrastructure::Logger::instance().error("Failed to parse blu-ray.com HTML");
        return items;
    }

    // The blu-ray.com release calendar page typically has a table with releases
    // We'll look for table rows containing release information
    // This is a basic implementation that may need adjustment based on actual HTML structure

    // For now, create a placeholder implementation that can be refined once we can access the page
    // The structure typically includes:
    // - Release date
    // - Title with link
    // - Format (Blu-ray, UHD 4K, 3D, etc.)
    // - Studio
    // - Cover image
    // - Price

    // TODO: Implement actual HTML parsing based on blu-ray.com structure
    // This requires testing with actual page content
    infrastructure::Logger::instance().warning(
        "BluRayComScraper parsing is not yet fully implemented. "
        "Manual testing required to determine HTML structure."
    );

    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return items;
}

std::optional<domain::ReleaseCalendarItem> BluRayComScraper::parseReleaseItem(
    const std::string& title,
    const std::string& release_date_str,
    const std::string& format,
    const std::string& studio,
    const std::string& image_url,
    const std::string& product_url,
    const std::string& price_str
) {
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
    } catch (const std::exception& e) {
        infrastructure::Logger::instance().error(
            std::format("Failed to parse release item: {}", e.what())
        );
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

std::chrono::system_clock::time_point BluRayComScraper::parseReleaseDate(std::string_view date_str) const {
    // Common date formats on blu-ray.com:
    // "Jan 15, 2024"
    // "2024-01-15"
    // "15 Jan 2024"

    std::tm tm = {};
    std::string date_str_copy{date_str};

    // Try different formats
    const char* formats[] = {
        "%b %d, %Y",   // Jan 15, 2024
        "%Y-%m-%d",    // 2024-01-15
        "%d %b %Y",    // 15 Jan 2024
        "%m/%d/%Y"     // 01/15/2024
    };

    for (const char* format : formats) {
        std::istringstream iss{date_str_copy};
        iss >> std::get_time(&tm, format);

        if (!iss.fail()) {
            auto time_t = std::mktime(&tm);
            return std::chrono::system_clock::from_time_t(time_t);
        }
    }

    // If parsing fails, return current time
    infrastructure::Logger::instance().warning(
        std::format("Failed to parse release date: {}, using current time", date_str)
    );
    return std::chrono::system_clock::now();
}

} // namespace bluray::application::scraper
