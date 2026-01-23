#pragma once

#include "../../domain/deal.hpp"
#include "../database_manager.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace bluray::infrastructure {

/**
 * Repository for deal persistence
 */
class DealRepository {
public:
  /**
   * Add a new deal
   */
  int64_t add(const domain::Deal &deal);

  /**
   * Update existing deal
   */
  bool update(const domain::Deal &deal);

  /**
   * Find deal by ID
   */
  std::optional<domain::Deal> findById(int64_t id);

  /**
   * Find deal by URL
   */
  std::optional<domain::Deal> findByUrl(std::string_view url);

  /**
   * Get all active deals
   */
  std::vector<domain::Deal> getAllActive();

  /**
   * Get active deals with pagination
   */
  std::vector<domain::Deal> getActive(int page, int page_size);

  /**
   * Get total count of active deals
   */
  int getActiveCount();

  /**
   * Get deals filtered by criteria
   */
  std::vector<domain::Deal> getFiltered(bool only_4k, std::string_view source,
                                        double min_discount, int page,
                                        int page_size);

  /**
   * Get filtered count
   */
  int getFilteredCount(bool only_4k, std::string_view source,
                       double min_discount);

  /**
   * Mark deal as inactive
   */
  bool markInactive(int64_t id);

  /**
   * Delete deal
   */
  bool remove(int64_t id);

  /**
   * Mark all expired deals as inactive
   */
  int markExpiredInactive();

private:
  domain::Deal dealFromStatement(Statement &stmt);
  std::string
  timePointToString(const std::chrono::system_clock::time_point &tp);
  std::chrono::system_clock::time_point
  stringToTimePoint(const std::string &str);
};

} // namespace bluray::infrastructure
