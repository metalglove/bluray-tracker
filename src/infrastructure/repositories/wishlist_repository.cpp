#include "wishlist_repository.hpp"
#include "../database_manager.hpp"
#include "../logger.hpp"
#include "../input_validation.hpp"
#include <fmt/format.h>
#include <iomanip>
#include <sstream>

namespace bluray::infrastructure::repositories {

using validation::isValidValue;
using validation::VALID_SORT_FIELDS;
using validation::VALID_SORT_ORDERS;
using validation::VALID_STOCK_FILTERS;

int SqliteWishlistRepository::add(const domain::WishlistItem &item) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        INSERT INTO wishlist (
            url, title, title_locked, current_price, desired_max_price, in_stock, is_uhd_4k,
            image_url, local_image_path, source, notify_on_price_drop, notify_on_stock,
            created_at, last_checked
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

  const auto created_at_str = timePointToString(item.created_at);
  const auto last_checked_str = timePointToString(item.last_checked);

  sqlite3_bind_text(stmt.get(), 1, item.url.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, item.title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 3, item.title_locked ? 1 : 0);
  sqlite3_bind_double(stmt.get(), 4, item.current_price);
  sqlite3_bind_double(stmt.get(), 5, item.desired_max_price);
  sqlite3_bind_int(stmt.get(), 6, item.in_stock ? 1 : 0);
  sqlite3_bind_int(stmt.get(), 7, item.is_uhd_4k ? 1 : 0);
  sqlite3_bind_text(stmt.get(), 8, item.image_url.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 9, item.local_image_path.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 10, item.source.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 11, item.notify_on_price_drop ? 1 : 0);
  sqlite3_bind_int(stmt.get(), 12, item.notify_on_stock ? 1 : 0);
  sqlite3_bind_text(stmt.get(), 13, created_at_str.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 14, last_checked_str.c_str(), -1,
                    SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    Logger::instance().error(fmt::format("Failed to insert wishlist item: {}",
                                         sqlite3_errmsg(db.getHandle())));
    return -1;
  }

  return static_cast<int>(db.lastInsertRowId());
}

bool SqliteWishlistRepository::update(const domain::WishlistItem &item) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        UPDATE wishlist SET
            title = ?, title_locked = ?, current_price = ?, desired_max_price = ?, in_stock = ?,
            is_uhd_4k = ?, image_url = ?, local_image_path = ?, source = ?,
            notify_on_price_drop = ?, notify_on_stock = ?, last_checked = ?
        WHERE id = ?
    )");

  const auto last_checked_str = timePointToString(item.last_checked);

  sqlite3_bind_text(stmt.get(), 1, item.title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 2, item.title_locked ? 1 : 0);
  sqlite3_bind_double(stmt.get(), 3, item.current_price);
  sqlite3_bind_double(stmt.get(), 4, item.desired_max_price);
  sqlite3_bind_int(stmt.get(), 5, item.in_stock ? 1 : 0);
  sqlite3_bind_int(stmt.get(), 6, item.is_uhd_4k ? 1 : 0);
  sqlite3_bind_text(stmt.get(), 7, item.image_url.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 8, item.local_image_path.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 9, item.source.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 10, item.notify_on_price_drop ? 1 : 0);
  sqlite3_bind_int(stmt.get(), 11, item.notify_on_stock ? 1 : 0);
  sqlite3_bind_text(stmt.get(), 12, last_checked_str.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 13, item.id);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

// Helper constant for consistent column ordering
static const std::string kColumnList =
    "id, url, title, title_locked, current_price, desired_max_price, in_stock, "
    "is_uhd_4k, "
    "image_url, local_image_path, source, notify_on_price_drop, "
    "notify_on_stock, "
    "created_at, last_checked";

bool SqliteWishlistRepository::remove(int id) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("DELETE FROM wishlist WHERE id = ?");
  sqlite3_bind_int(stmt.get(), 1, id);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

std::optional<domain::WishlistItem> SqliteWishlistRepository::findById(int id) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto query = fmt::format("SELECT {} FROM wishlist WHERE id = ?", kColumnList);
  auto stmt = db.prepare(query);
  sqlite3_bind_int(stmt.get(), 1, id);

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return fromStatement(stmt.get());
  }

  return std::nullopt;
}

std::optional<domain::WishlistItem>
SqliteWishlistRepository::findByUrl(std::string_view url) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto query =
      fmt::format("SELECT {} FROM wishlist WHERE url = ?", kColumnList);
  auto stmt = db.prepare(query);
  sqlite3_bind_text(stmt.get(), 1, std::string(url).c_str(), -1,
                    SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return fromStatement(stmt.get());
  }

  return std::nullopt;
}

std::vector<domain::WishlistItem> SqliteWishlistRepository::findAll() {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto query = fmt::format("SELECT {} FROM wishlist ORDER BY created_at DESC",
                           kColumnList);
  auto stmt = db.prepare(query);

  std::vector<domain::WishlistItem> items;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    items.push_back(fromStatement(stmt.get()));
  }

  return items;
}

domain::PaginatedResult<domain::WishlistItem>
SqliteWishlistRepository::findAll(const domain::PaginationParams &params) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  domain::PaginatedResult<domain::WishlistItem> result;
  result.page = params.page;
  result.page_size = params.page_size;

  std::vector<std::string> conditions;
  std::vector<std::string> bind_params; // Keep track of what we need to bind

  // Validate and apply stock filter
  if (!params.filter_stock.empty()) {
    if (!isValidValue(params.filter_stock, VALID_STOCK_FILTERS)) {
      Logger::instance().warn(
          fmt::format("Invalid filter_stock value: {}", params.filter_stock));
    } else {
      if (params.filter_stock == "in_stock") {
        conditions.push_back("in_stock = 1");
      } else if (params.filter_stock == "out_of_stock") {
        conditions.push_back("in_stock = 0");
      }
    }
  }

  // filter_source and search_query use parameterized queries (safe)
  if (!params.filter_source.empty()) {
    conditions.push_back("source = ?");
    bind_params.push_back(params.filter_source);
  }

  if (!params.search_query.empty()) {
    conditions.push_back("title LIKE ?");
    bind_params.push_back("%" + params.search_query + "%");
  }

  // Validate and apply sorting with whitelist
  std::string order_clause = "ORDER BY created_at DESC";
  if (!params.sort_by.empty()) {
    if (!isValidValue(params.sort_by, VALID_SORT_FIELDS)) {
      Logger::instance().warn(
          fmt::format("Invalid sort_by value: {}, using default", params.sort_by));
    } else {
      // Validate sort_order
      std::string direction = "DESC";
      if (!params.sort_order.empty()) {
        if (!isValidValue(params.sort_order, VALID_SORT_ORDERS)) {
          Logger::instance().warn(fmt::format(
              "Invalid sort_order value: {}, using DESC", params.sort_order));
        } else {
          direction = (params.sort_order == "desc") ? "DESC" : "ASC";
        }
      }

      // Build ORDER BY clause with validated values
      if (params.sort_by == "price") {
        order_clause = "ORDER BY current_price " + direction;
      } else if (params.sort_by == "title") {
        order_clause = "ORDER BY title " + direction;
      } else if (params.sort_by == "date") {
        order_clause = "ORDER BY created_at " + direction;
      }
    }
  }

  std::string where_clause;
  if (!conditions.empty()) {
    where_clause = "WHERE " + conditions[0];
    for (size_t i = 1; i < conditions.size(); ++i) {
      where_clause += " AND " + conditions[i];
    }
  }

  // Count with filters
  std::string count_query = "SELECT COUNT(*) FROM wishlist " + where_clause;
  auto count_stmt = db.prepare(count_query);

  int bind_idx = 1;
  for (const auto &param : bind_params) {
    sqlite3_bind_text(count_stmt.get(), bind_idx++, param.c_str(), -1,
                      SQLITE_TRANSIENT);
  }

  if (sqlite3_step(count_stmt.get()) == SQLITE_ROW) {
    result.total_count = sqlite3_column_int(count_stmt.get(), 0);
  }

  auto query = fmt::format("SELECT {} FROM wishlist {} {} LIMIT ? OFFSET ?",
                           kColumnList, where_clause, order_clause);
  auto stmt = db.prepare(query);

  bind_idx = 1;
  for (const auto &param : bind_params) {
    sqlite3_bind_text(stmt.get(), bind_idx++, param.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  sqlite3_bind_int(stmt.get(), bind_idx++, params.limit());
  sqlite3_bind_int(stmt.get(), bind_idx++, params.offset());

  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    result.items.push_back(fromStatement(stmt.get()));
  }

  return result;
}

int SqliteWishlistRepository::count() {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT COUNT(*) FROM wishlist");

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return sqlite3_column_int(stmt.get(), 0);
  }

  return 0;
}

domain::WishlistItem
SqliteWishlistRepository::fromStatement(sqlite3_stmt *stmt) {
  domain::WishlistItem item;

  item.id = sqlite3_column_int(stmt, 0);
  item.url = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
  item.title = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
  item.title_locked = sqlite3_column_int(stmt, 3) != 0;
  item.current_price = sqlite3_column_double(stmt, 4);
  item.desired_max_price = sqlite3_column_double(stmt, 5);
  item.in_stock = sqlite3_column_int(stmt, 6) != 0;
  item.is_uhd_4k = sqlite3_column_int(stmt, 7) != 0;

  if (const char *image_url =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8))) {
    item.image_url = image_url;
  }
  if (const char *local_path =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 9))) {
    item.local_image_path = local_path;
  }

  item.source = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 10));
  item.notify_on_price_drop = sqlite3_column_int(stmt, 11) != 0;
  item.notify_on_stock = sqlite3_column_int(stmt, 12) != 0;

  const char *created_at =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, 13));
  const char *last_checked =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, 14));

  item.created_at = stringToTimePoint(created_at);
  item.last_checked = stringToTimePoint(last_checked);

  return item;
}

std::string SqliteWishlistRepository::timePointToString(
    const std::chrono::system_clock::time_point &tp) {
  const auto time_t = std::chrono::system_clock::to_time_t(tp);
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

std::chrono::system_clock::time_point
SqliteWishlistRepository::stringToTimePoint(const std::string &str) {
  std::tm tm = {};
  std::istringstream iss(str);
  iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

  const auto time_t = std::mktime(&tm);
  return std::chrono::system_clock::from_time_t(time_t);
}

} // namespace bluray::infrastructure::repositories
