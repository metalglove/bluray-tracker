#pragma once

#include "../../domain/deal.hpp"
#include "../../domain/models.hpp"
#include "../../infrastructure/repositories/deal_repository.hpp"
#include "../../infrastructure/repositories/wishlist_repository.hpp"

#include <memory>
#include <vector>

namespace bluray::application {

/**
 * Service for detecting and managing deals
 */
class DealsService {
public:
  /**
   * Detect if a product qualifies as a deal
   * Returns deal object if criteria met, nullopt otherwise
   */
  std::optional<domain::Deal>
  detectDeal(const domain::Product &product, double historical_low = 0.0);

  /**
   * Calculate deal score (0-100, higher is better)
   * Factors: discount %, price drop, historical context
   */
  double calculateDealScore(const domain::Deal &deal,
                            double historical_low = 0.0);

  /**
   * Check if deal matches any wishlist items
   * Returns wishlist item IDs that match
   */
  std::vector<int64_t> findMatchingWishlistItems(const domain::Deal &deal);

  /**
   * Check if deal is still valid (price/stock haven't changed)
   */
  bool validateDeal(const domain::Deal &deal);

  /**
   * Determine deal type based on various factors
   */
  std::string determineDealType(double discount_pct,
                                const std::optional<std::chrono::system_clock::time_point> &ends_at);

private:
  // Deal detection thresholds
  static constexpr double MIN_DISCOUNT_PERCENTAGE = 15.0; // 15% minimum
  static constexpr double GREAT_DEAL_THRESHOLD = 30.0;    // 30%+ is great
  static constexpr double AMAZING_DEAL_THRESHOLD = 50.0;  // 50%+ is amazing
  static constexpr double MAX_DEAL_PRICE = 50.0; // €50 max for deals
};

} // namespace bluray::application
