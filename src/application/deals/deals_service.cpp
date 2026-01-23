#include "deals_service.hpp"
#include "../../infrastructure/logger.hpp"

#include <algorithm>
#include <fmt/format.h>

namespace bluray::application {

std::optional<domain::Deal>
DealsService::detectDeal(const domain::Product &product,
                         double historical_low) {
  // Must have a valid price
  if (product.price <= 0.0) {
    return std::nullopt;
  }

  // Must be in stock
  if (!product.in_stock) {
    return std::nullopt;
  }

  // Estimate original price (either from historical data or heuristic)
  double original_price = product.price;

  // If we have historical low, use it as reference
  if (historical_low > 0.0 && historical_low < product.price) {
    // Current price is higher than historical low - might not be a great deal
    original_price = historical_low * 1.3; // Assume 30% markup as "original"
  } else if (historical_low > 0.0) {
    // Current price is at or below historical low - good deal!
    original_price = historical_low * 1.5; // Assume 50% markup
  } else {
    // No historical data - use heuristic based on format
    if (product.is_uhd_4k) {
      original_price = product.price * 1.4; // Assume 40% discount from MSRP
    } else {
      original_price = product.price * 1.3; // Assume 30% discount
    }
  }

  // Calculate discount percentage
  const double discount_pct =
      ((original_price - product.price) / original_price) * 100.0;

  // Check if it meets minimum deal criteria
  if (discount_pct < MIN_DISCOUNT_PERCENTAGE) {
    return std::nullopt;
  }

  // Check price cap
  if (product.price > MAX_DEAL_PRICE) {
    // Only report high-price items if discount is very significant
    if (discount_pct < GREAT_DEAL_THRESHOLD) {
      return std::nullopt;
    }
  }

  // Create deal object
  domain::Deal deal;
  deal.url = product.url;
  deal.title = product.title;
  deal.source = product.source;
  deal.original_price = original_price;
  deal.deal_price = product.price;
  deal.discount_percentage = discount_pct;
  deal.is_uhd_4k = product.is_uhd_4k;
  deal.image_url = product.image_url;
  deal.local_image_path = product.local_image_path;
  deal.discovered_at = std::chrono::system_clock::now();
  deal.last_checked = std::chrono::system_clock::now();
  deal.is_active = true;

  // Determine deal type (would need additional scraping for end times)
  deal.deal_type = determineDealType(discount_pct, std::nullopt);

  infrastructure::Logger::instance().info(
      fmt::format("Detected deal: {} - {:.0f}% off (€{:.2f} -> €{:.2f})",
                  deal.title, discount_pct, original_price, product.price));

  return deal;
}

double DealsService::calculateDealScore(const domain::Deal &deal,
                                        double historical_low) {
  double score = 0.0;

  // Base score from discount percentage (0-50 points)
  score += std::min(deal.discount_percentage, 50.0);

  // Bonus for deep discounts (0-20 points)
  if (deal.discount_percentage >= AMAZING_DEAL_THRESHOLD) {
    score += 20.0;
  } else if (deal.discount_percentage >= GREAT_DEAL_THRESHOLD) {
    score += 10.0;
  }

  // Bonus for 4K UHD (0-10 points)
  if (deal.is_uhd_4k) {
    score += 10.0;
  }

  // Bonus for price relative to historical low (0-15 points)
  if (historical_low > 0.0) {
    const double price_ratio = deal.deal_price / historical_low;
    if (price_ratio <= 1.0) {
      score += 15.0; // At or below historical low
    } else if (price_ratio <= 1.1) {
      score += 10.0; // Within 10% of historical low
    } else if (price_ratio <= 1.2) {
      score += 5.0; // Within 20% of historical low
    }
  }

  // Penalty for high absolute price (0-5 points deduction)
  if (deal.deal_price > 30.0) {
    score -= 5.0;
  }

  return std::max(0.0, std::min(100.0, score));
}

std::vector<int64_t>
DealsService::findMatchingWishlistItems(const domain::Deal &deal) {
  std::vector<int64_t> matches;

  infrastructure::repositories::SqliteWishlistRepository wishlist_repo;
  auto wishlist_items = wishlist_repo.findAll();

  for (const auto &item : wishlist_items) {
    // Check for URL match (exact)
    if (item.url == deal.url) {
      matches.push_back(item.id);
      continue;
    }

    // Check for title similarity (fuzzy match)
    // Simple approach: check if deal title contains wishlist title or vice
    // versa
    std::string deal_title_lower = deal.title;
    std::string item_title_lower = item.title;

    std::transform(deal_title_lower.begin(), deal_title_lower.end(),
                   deal_title_lower.begin(), ::tolower);
    std::transform(item_title_lower.begin(), item_title_lower.end(),
                   item_title_lower.begin(), ::tolower);

    if (deal_title_lower.find(item_title_lower) != std::string::npos ||
        item_title_lower.find(deal_title_lower) != std::string::npos) {
      // Also check format matches
      if (deal.is_uhd_4k == item.is_uhd_4k) {
        matches.push_back(item.id);
      }
    }
  }

  return matches;
}

bool DealsService::validateDeal(const domain::Deal &deal) {
  // This would need actual scraping to verify current price/stock
  // For now, just check if deal hasn't expired
  if (deal.isExpired()) {
    return false;
  }

  return deal.is_active;
}

std::string DealsService::determineDealType(
    double discount_pct,
    const std::optional<std::chrono::system_clock::time_point> &ends_at) {
  if (ends_at) {
    auto now = std::chrono::system_clock::now();
    auto remaining_hours =
        std::chrono::duration_cast<std::chrono::hours>(*ends_at - now).count();

    if (remaining_hours <= 6) {
      return "lightning"; // Expires within 6 hours
    }
    return "daily"; // Has expiration but > 6 hours
  }

  if (discount_pct >= AMAZING_DEAL_THRESHOLD) {
    return "promotion"; // Major discount, likely a promotion
  }

  return "daily"; // Default type
}

} // namespace bluray::application
