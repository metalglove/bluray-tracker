#include "release_calendar_repository.hpp"
#include "../database_manager.hpp"
#include "../logger.hpp"
#include <fmt/format.h>
#include <iomanip>
#include <sstream>

namespace bluray::infrastructure::repositories {

int SqliteReleaseCalendarRepository::add(
    const domain::ReleaseCalendarItem &item) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        INSERT INTO release_calendar (
            title, release_date, format, studio, image_url, local_image_path,
            product_url, is_uhd_4k, is_preorder, price, notes, created_at, last_updated
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

  const auto release_date_str = timePointToString(item.release_date);
  const auto created_at_str = timePointToString(item.created_at);
  const auto last_updated_str = timePointToString(item.last_updated);

  sqlite3_bind_text(stmt.get(), 1, item.title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, release_date_str.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, item.format.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, item.studio.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 5, item.image_url.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 6, item.local_image_path.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 7, item.product_url.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 8, item.is_uhd_4k ? 1 : 0);
  sqlite3_bind_int(stmt.get(), 9, item.is_preorder ? 1 : 0);
  sqlite3_bind_double(stmt.get(), 10, item.price);
  sqlite3_bind_text(stmt.get(), 11, item.notes.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 12, created_at_str.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 13, last_updated_str.c_str(), -1,
                    SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    Logger::instance().error(
        fmt::format("Failed to insert release calendar item: {}",
                    sqlite3_errmsg(db.getHandle())));
    return -1;
  }

  return static_cast<int>(db.lastInsertRowId());
}

bool SqliteReleaseCalendarRepository::update(
    const domain::ReleaseCalendarItem &item) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        UPDATE release_calendar SET
            title = ?, release_date = ?, format = ?, studio = ?, image_url = ?,
            local_image_path = ?, product_url = ?, is_uhd_4k = ?, is_preorder = ?,
            price = ?, notes = ?, last_updated = ?
        WHERE id = ?
    )");

  const auto release_date_str = timePointToString(item.release_date);
  const auto last_updated_str = timePointToString(item.last_updated);

  sqlite3_bind_text(stmt.get(), 1, item.title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, release_date_str.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, item.format.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, item.studio.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 5, item.image_url.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 6, item.local_image_path.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 7, item.product_url.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 8, item.is_uhd_4k ? 1 : 0);
  sqlite3_bind_int(stmt.get(), 9, item.is_preorder ? 1 : 0);
  sqlite3_bind_double(stmt.get(), 10, item.price);
  sqlite3_bind_text(stmt.get(), 11, item.notes.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 12, last_updated_str.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 13, item.id);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

bool SqliteReleaseCalendarRepository::remove(int id) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("DELETE FROM release_calendar WHERE id = ?");
  sqlite3_bind_int(stmt.get(), 1, id);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

std::optional<domain::ReleaseCalendarItem>
SqliteReleaseCalendarRepository::findById(int id) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT * FROM release_calendar WHERE id = ?");
  sqlite3_bind_int(stmt.get(), 1, id);

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return fromStatement(stmt.get());
  }

  return std::nullopt;
}

std::optional<domain::ReleaseCalendarItem>
SqliteReleaseCalendarRepository::findByUrl(std::string_view url) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt =
      db.prepare("SELECT * FROM release_calendar WHERE product_url = ?");
  sqlite3_bind_text(stmt.get(), 1, std::string(url).c_str(), -1,
                    SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return fromStatement(stmt.get());
  }

  return std::nullopt;
}

std::vector<domain::ReleaseCalendarItem>
SqliteReleaseCalendarRepository::findAll() {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt =
      db.prepare("SELECT * FROM release_calendar ORDER BY release_date ASC");

  std::vector<domain::ReleaseCalendarItem> items;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    items.push_back(fromStatement(stmt.get()));
  }

  return items;
}

domain::PaginatedResult<domain::ReleaseCalendarItem>
SqliteReleaseCalendarRepository::findAll(
    const domain::PaginationParams &params) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  domain::PaginatedResult<domain::ReleaseCalendarItem> result;
  result.page = params.page;
  result.page_size = params.page_size;
  result.total_count = count();

  auto stmt = db.prepare("SELECT * FROM release_calendar ORDER BY release_date "
                         "ASC LIMIT ? OFFSET ?");
  sqlite3_bind_int(stmt.get(), 1, params.limit());
  sqlite3_bind_int(stmt.get(), 2, params.offset());

  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    result.items.push_back(fromStatement(stmt.get()));
  }

  return result;
}

std::vector<domain::ReleaseCalendarItem>
SqliteReleaseCalendarRepository::findByDateRange(
    const std::chrono::system_clock::time_point &start,
    const std::chrono::system_clock::time_point &end) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT * FROM release_calendar WHERE release_date >= "
                         "? AND release_date <= ? ORDER BY release_date ASC");

  const auto start_str = timePointToString(start);
  const auto end_str = timePointToString(end);

  sqlite3_bind_text(stmt.get(), 1, start_str.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, end_str.c_str(), -1, SQLITE_TRANSIENT);

  std::vector<domain::ReleaseCalendarItem> items;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    items.push_back(fromStatement(stmt.get()));
  }

  return items;
}

int SqliteReleaseCalendarRepository::count() {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT COUNT(*) FROM release_calendar");

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return sqlite3_column_int(stmt.get(), 0);
  }

  return 0;
}

int SqliteReleaseCalendarRepository::removeOlderThan(
    const std::chrono::system_clock::time_point &cutoff_date) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  const auto cutoff_str = timePointToString(cutoff_date);

  auto stmt = db.prepare("DELETE FROM release_calendar WHERE release_date < ?");
  sqlite3_bind_text(stmt.get(), 1, cutoff_str.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) == SQLITE_DONE) {
    return sqlite3_changes(db.getHandle());
  }

  return 0;
}

domain::ReleaseCalendarItem
SqliteReleaseCalendarRepository::fromStatement(sqlite3_stmt *stmt) {
  domain::ReleaseCalendarItem item;

  item.id = sqlite3_column_int(stmt, 0);
  item.title = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

  const char *release_date =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
  item.release_date = stringToTimePoint(release_date);

  item.format = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));

  if (const char *studio =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4))) {
    item.studio = studio;
  }
  if (const char *image_url =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5))) {
    item.image_url = image_url;
  }
  if (const char *local_path =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6))) {
    item.local_image_path = local_path;
  }
  if (const char *product_url =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7))) {
    item.product_url = product_url;
  }

  item.is_uhd_4k = sqlite3_column_int(stmt, 8) != 0;
  item.is_preorder = sqlite3_column_int(stmt, 9) != 0;
  item.price = sqlite3_column_double(stmt, 10);

  if (const char *notes =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 11))) {
    item.notes = notes;
  }

  const char *created_at =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, 12));
  const char *last_updated =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, 13));

  item.created_at = stringToTimePoint(created_at);
  item.last_updated = stringToTimePoint(last_updated);

  return item;
}

std::string SqliteReleaseCalendarRepository::timePointToString(
    const std::chrono::system_clock::time_point &tp) {
  const auto time_t = std::chrono::system_clock::to_time_t(tp);
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

std::chrono::system_clock::time_point
SqliteReleaseCalendarRepository::stringToTimePoint(const std::string &str) {
  std::tm tm = {};
  std::istringstream iss(str);
  iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

  const auto time_t = std::mktime(&tm);
  return std::chrono::system_clock::from_time_t(time_t);
}

} // namespace bluray::infrastructure::repositories
