#pragma once

#include "../../domain/models.hpp"
#include <vector>
#include <optional>
#include <sqlite3.h>

namespace bluray::infrastructure::repositories {

/**
 * Repository interface for price history operations
 */
class IPriceHistoryRepository {
public:
    virtual ~IPriceHistoryRepository() = default;

    virtual int add(const domain::PriceHistoryEntry& entry) = 0;
    virtual std::vector<domain::PriceHistoryEntry> findByWishlistId(int wishlist_id, int days = 180) = 0;
    virtual void pruneOldEntries(int days = 180) = 0;
};

/**
 * SQLite implementation of price history repository
 */
class SqlitePriceHistoryRepository : public IPriceHistoryRepository {
public:
    int add(const domain::PriceHistoryEntry& entry) override;
    std::vector<domain::PriceHistoryEntry> findByWishlistId(int wishlist_id, int days = 180) override;
    void pruneOldEntries(int days = 180) override;

private:
    static domain::PriceHistoryEntry fromStatement(sqlite3_stmt* stmt);
    static std::string timePointToString(const std::chrono::system_clock::time_point& tp);
    static std::chrono::system_clock::time_point stringToTimePoint(const std::string& str);
};

} // namespace bluray::infrastructure::repositories
