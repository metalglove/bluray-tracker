#include "price_history_repository.hpp"
#include "../database_manager.hpp"
#include "../logger.hpp"
#include <format>
#include <iomanip>
#include <sstream>

namespace bluray::infrastructure::repositories {

int SqlitePriceHistoryRepository::add(const domain::PriceHistoryEntry& entry) {
    auto& db = DatabaseManager::instance();
    auto lock = db.lock();

    auto stmt = db.prepare(R"(
        INSERT INTO price_history (wishlist_id, price, in_stock, recorded_at)
        VALUES (?, ?, ?, ?)
    )");

    const auto recorded_at_str = timePointToString(entry.recorded_at);

    sqlite3_bind_int(stmt.get(), 1, entry.wishlist_id);
    sqlite3_bind_double(stmt.get(), 2, entry.price);
    sqlite3_bind_int(stmt.get(), 3, entry.in_stock ? 1 : 0);
    sqlite3_bind_text(stmt.get(), 4, recorded_at_str.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        Logger::instance().error(
            std::format("Failed to insert price history entry: {}", sqlite3_errmsg(db.getHandle()))
        );
        return -1;
    }

    return static_cast<int>(db.lastInsertRowId());
}

std::vector<domain::PriceHistoryEntry> SqlitePriceHistoryRepository::findByWishlistId(int wishlist_id, int days) {
    auto& db = DatabaseManager::instance();
    auto lock = db.lock();

    auto stmt = db.prepare(R"(
        SELECT * FROM price_history
        WHERE wishlist_id = ?
        AND recorded_at >= datetime('now', '-' || ? || ' days')
        ORDER BY recorded_at ASC
    )");

    sqlite3_bind_int(stmt.get(), 1, wishlist_id);
    sqlite3_bind_int(stmt.get(), 2, days);

    std::vector<domain::PriceHistoryEntry> entries;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        entries.push_back(fromStatement(stmt.get()));
    }

    return entries;
}

void SqlitePriceHistoryRepository::pruneOldEntries(int days) {
    auto& db = DatabaseManager::instance();
    auto lock = db.lock();

    auto stmt = db.prepare(R"(
        DELETE FROM price_history
        WHERE recorded_at < datetime('now', '-' || ? || ' days')
    )");

    sqlite3_bind_int(stmt.get(), 1, days);

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        Logger::instance().error(
            std::format("Failed to prune price history: {}", sqlite3_errmsg(db.getHandle()))
        );
    } else {
        const int deleted = sqlite3_changes(db.getHandle());
        Logger::instance().info(std::format("Pruned {} old price history entries", deleted));
    }
}

domain::PriceHistoryEntry SqlitePriceHistoryRepository::fromStatement(sqlite3_stmt* stmt) {
    domain::PriceHistoryEntry entry;

    entry.id = sqlite3_column_int(stmt, 0);
    entry.wishlist_id = sqlite3_column_int(stmt, 1);
    entry.price = sqlite3_column_double(stmt, 2);
    entry.in_stock = sqlite3_column_int(stmt, 3) != 0;

    const char* recorded_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    if (recorded_at) {
        entry.recorded_at = stringToTimePoint(recorded_at);
    }

    return entry;
}

std::string SqlitePriceHistoryRepository::timePointToString(const std::chrono::system_clock::time_point& tp) {
    const auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::chrono::system_clock::time_point SqlitePriceHistoryRepository::stringToTimePoint(const std::string& str) {
    std::tm tm{};
    std::istringstream ss(str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

} // namespace bluray::infrastructure::repositories
