#include "price_history_repository.hpp"
#include "../database_manager.hpp"
#include "../logger.hpp"
#include <fmt/format.h>

namespace bluray::infrastructure {

void PriceHistoryRepository::addEntry(int wishlist_id, double price,
                                      bool in_stock) {
  auto &db = DatabaseManager::instance();

  // Check if the last entry for this item has the same price and stock status.
  // If so, we might want to skip inserting to save space, OR we insert anyway
  // to show confirmation. For a price graph, usually we want at least one point
  // per day or change. Let's implement a simple optimize: if the last entry was
  // recent (e.g. < 6 hours) AND price/stock is same, skip. But for now, as per
  // roadmap, strictly follow the instruction: insert records. Optimization can
  // be added later or we can rely on the graph to handle many points.

  // Actually, looking at the roadmap plan: "In runOnce(), after detecting price
  // changes, insert records". But wait, if we only insert on change, the graph
  // might look empty if price is stable. A better approach for a "History"
  // graph is to record periodically (e.g. once a day) OR whenever it changes.
  // For simplicity and fulfilling the "monitor" aspect, let's just insert every
  // time we scrape for now, or maybe the logic belongs in the Scheduler. The
  // Repository just does what it is told.

  try {
    auto stmt = db.prepare(
        "INSERT INTO price_history (wishlist_id, price, in_stock, recorded_at) "
        "VALUES (?, ?, ?, datetime('now'))");

    sqlite3_bind_int(stmt.get(), 1, wishlist_id);
    sqlite3_bind_double(stmt.get(), 2, price);
    sqlite3_bind_int(stmt.get(), 3, in_stock ? 1 : 0);

    sqlite3_step(stmt.get());
  } catch (const std::exception &e) {
    Logger::instance().error(
        fmt::format("Failed to add price history: {}", e.what()));
  }
}

std::vector<PriceHistoryEntry>
PriceHistoryRepository::getHistory(int wishlist_id, int days) {
  std::vector<PriceHistoryEntry> history;
  auto &db = DatabaseManager::instance();

  try {
    auto stmt = db.prepare(
        "SELECT id, wishlist_id, price, in_stock, recorded_at "
        "FROM price_history "
        "WHERE wishlist_id = ? AND recorded_at >= datetime('now', ?) "
        "ORDER BY recorded_at ASC");

    sqlite3_bind_int(stmt.get(), 1, wishlist_id);
    std::string days_param = fmt::format("-{} days", days);
    sqlite3_bind_text(stmt.get(), 2, days_param.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
      PriceHistoryEntry entry;
      entry.id = sqlite3_column_int(stmt.get(), 0);
      entry.wishlist_id = sqlite3_column_int(stmt.get(), 1);
      entry.price = sqlite3_column_double(stmt.get(), 2);
      entry.in_stock = sqlite3_column_int(stmt.get(), 3) != 0;
      entry.recorded_at =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 4));

      history.push_back(entry);
    }
  } catch (const std::exception &e) {
    Logger::instance().error(
        fmt::format("Failed to get price history: {}", e.what()));
  }

  return history;
}

void PriceHistoryRepository::pruneHistory(int days_to_keep) {
  auto &db = DatabaseManager::instance();

  try {
    auto stmt = db.prepare(
        "DELETE FROM price_history WHERE recorded_at < datetime('now', ?)");

    std::string days_param = fmt::format("-{} days", days_to_keep);
    sqlite3_bind_text(stmt.get(), 1, days_param.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt.get());
  } catch (const std::exception &e) {
    Logger::instance().error(
        fmt::format("Failed to prune price history: {}", e.what()));
  }
}

} // namespace bluray::infrastructure
