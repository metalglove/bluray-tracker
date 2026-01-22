#include "collection_repository.hpp"
#include "../database_manager.hpp"
#include "../logger.hpp"
#include "../input_validation.hpp"
#include <fmt/format.h>
#include <iomanip>
#include <sstream>

namespace bluray::infrastructure::repositories {

int SqliteCollectionRepository::add(const domain::CollectionItem &item) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        INSERT INTO collection (
            url, title, purchase_price, is_uhd_4k, image_url, local_image_path,
            source, notes, purchased_at, added_at, tmdb_id, imdb_id, tmdb_rating,
            trailer_key, edition_type, has_slipcover, has_digital_copy, bonus_features
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

  const auto purchased_at_str = timePointToString(item.purchased_at);
  const auto added_at_str = timePointToString(item.added_at);

  sqlite3_bind_text(stmt.get(), 1, item.url.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, item.title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt.get(), 3, item.purchase_price);
  sqlite3_bind_int(stmt.get(), 4, item.is_uhd_4k ? 1 : 0);
  sqlite3_bind_text(stmt.get(), 5, item.image_url.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 6, item.local_image_path.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 7, item.source.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 8, item.notes.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 9, purchased_at_str.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 10, added_at_str.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 11, item.tmdb_id);
  sqlite3_bind_text(stmt.get(), 12, item.imdb_id.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt.get(), 13, item.tmdb_rating);
  sqlite3_bind_text(stmt.get(), 14, item.trailer_key.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 15, item.edition_type.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 16, item.has_slipcover ? 1 : 0);
  sqlite3_bind_int(stmt.get(), 17, item.has_digital_copy ? 1 : 0);
  sqlite3_bind_text(stmt.get(), 18, item.bonus_features.c_str(), -1,
                    SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    Logger::instance().error(fmt::format("Failed to insert collection item: {}",
                                         sqlite3_errmsg(db.getHandle())));
    return -1;
  }

  return static_cast<int>(db.lastInsertRowId());
}

bool SqliteCollectionRepository::update(const domain::CollectionItem &item) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        UPDATE collection SET
            title = ?, purchase_price = ?, is_uhd_4k = ?, image_url = ?,
            local_image_path = ?, source = ?, notes = ?, purchased_at = ?,
            tmdb_id = ?, imdb_id = ?, tmdb_rating = ?, trailer_key = ?,
            edition_type = ?, has_slipcover = ?, has_digital_copy = ?, bonus_features = ?
        WHERE id = ?
    )");

  const auto purchased_at_str = timePointToString(item.purchased_at);

  sqlite3_bind_text(stmt.get(), 1, item.title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt.get(), 2, item.purchase_price);
  sqlite3_bind_int(stmt.get(), 3, item.is_uhd_4k ? 1 : 0);
  sqlite3_bind_text(stmt.get(), 4, item.image_url.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 5, item.local_image_path.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 6, item.source.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 7, item.notes.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 8, purchased_at_str.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 9, item.tmdb_id);
  sqlite3_bind_text(stmt.get(), 10, item.imdb_id.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt.get(), 11, item.tmdb_rating);
  sqlite3_bind_text(stmt.get(), 12, item.trailer_key.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 13, item.edition_type.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 14, item.has_slipcover ? 1 : 0);
  sqlite3_bind_int(stmt.get(), 15, item.has_digital_copy ? 1 : 0);
  sqlite3_bind_text(stmt.get(), 16, item.bonus_features.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 17, item.id);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

bool SqliteCollectionRepository::remove(int id) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("DELETE FROM collection WHERE id = ?");
  sqlite3_bind_int(stmt.get(), 1, id);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

std::optional<domain::CollectionItem>
SqliteCollectionRepository::findById(int id) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT * FROM collection WHERE id = ?");
  sqlite3_bind_int(stmt.get(), 1, id);

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return fromStatement(stmt.get());
  }

  return std::nullopt;
}

std::optional<domain::CollectionItem>
SqliteCollectionRepository::findByUrl(std::string_view url) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT * FROM collection WHERE url = ?");
  sqlite3_bind_text(stmt.get(), 1, std::string(url).c_str(), -1,
                    SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return fromStatement(stmt.get());
  }

  return std::nullopt;
}

std::vector<domain::CollectionItem> SqliteCollectionRepository::findAll() {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT * FROM collection ORDER BY added_at DESC");

  std::vector<domain::CollectionItem> items;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    items.push_back(fromStatement(stmt.get()));
  }

  return items;
}

domain::PaginatedResult<domain::CollectionItem>
SqliteCollectionRepository::findAll(const domain::PaginationParams &params) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  domain::PaginatedResult<domain::CollectionItem> result;
  result.page = params.page;
  result.page_size = params.page_size;
  std::vector<std::string> conditions;
  
  // filter_source and search_query use parameterized queries (safe)
  if (!params.filter_source.empty()) {
    conditions.push_back("source = ?");
  }
  if (!params.search_query.empty()) {
    conditions.push_back("title LIKE ?");
  }

  std::string order_clause = "ORDER BY added_at DESC";
  // Collection sorting logic (if needed later), currently defaults to date
  // added.

  std::string where_clause;
  if (!conditions.empty()) {
    where_clause = "WHERE " + conditions[0];
    for (size_t i = 1; i < conditions.size(); ++i) {
      where_clause += " AND " + conditions[i];
    }
  }

  // Count with filters
  auto count_stmt =
      db.prepare("SELECT COUNT(*) FROM collection " + where_clause);
  int bind_idx = 1;
  if (!params.filter_source.empty()) {
    sqlite3_bind_text(count_stmt.get(), bind_idx++,
                      params.filter_source.c_str(), -1, SQLITE_TRANSIENT);
  }
  if (!params.search_query.empty()) {
    std::string query = "%" + params.search_query + "%";
    sqlite3_bind_text(count_stmt.get(), bind_idx++, query.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (sqlite3_step(count_stmt.get()) == SQLITE_ROW) {
    result.total_count = sqlite3_column_int(count_stmt.get(), 0);
  }

  // Total Value with filters
  auto value_stmt =
      db.prepare("SELECT SUM(purchase_price) FROM collection " + where_clause);
  bind_idx = 1;
  if (!params.filter_source.empty()) {
    sqlite3_bind_text(value_stmt.get(), bind_idx++,
                      params.filter_source.c_str(), -1, SQLITE_TRANSIENT);
  }
  if (!params.search_query.empty()) {
    std::string query = "%" + params.search_query + "%";
    sqlite3_bind_text(value_stmt.get(), bind_idx++, query.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (sqlite3_step(value_stmt.get()) == SQLITE_ROW) {
    result.total_value = sqlite3_column_double(value_stmt.get(), 0);
  }

  auto stmt = db.prepare("SELECT * FROM collection " + where_clause + " " +
                         order_clause + " LIMIT ? OFFSET ?");
  bind_idx = 1;
  if (!params.filter_source.empty()) {
    sqlite3_bind_text(stmt.get(), bind_idx++, params.filter_source.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (!params.search_query.empty()) {
    std::string query = "%" + params.search_query + "%";
    sqlite3_bind_text(stmt.get(), bind_idx++, query.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  sqlite3_bind_int(stmt.get(), bind_idx++, params.limit());
  sqlite3_bind_int(stmt.get(), bind_idx++, params.offset());

  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    result.items.push_back(fromStatement(stmt.get()));
  }

  return result;
}

int SqliteCollectionRepository::count() {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT COUNT(*) FROM collection");

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return sqlite3_column_int(stmt.get(), 0);
  }

  return 0;
}

double SqliteCollectionRepository::totalValue() {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT SUM(purchase_price) FROM collection");

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return sqlite3_column_double(stmt.get(), 0);
  }

  return 0.0;
}

domain::CollectionItem
SqliteCollectionRepository::fromStatement(sqlite3_stmt *stmt) {
  domain::CollectionItem item;

  item.id = sqlite3_column_int(stmt, 0);
  item.url = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
  item.title = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
  item.purchase_price = sqlite3_column_double(stmt, 3);
  item.is_uhd_4k = sqlite3_column_int(stmt, 4) != 0;

  if (const char *image_url =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5))) {
    item.image_url = image_url;
  }
  if (const char *local_path =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6))) {
    item.local_image_path = local_path;
  }

  item.source = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));

  if (const char *notes =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8))) {
    item.notes = notes;
  }

  const char *purchased_at =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, 9));
  const char *added_at =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, 10));

  item.purchased_at = stringToTimePoint(purchased_at);
  item.added_at = stringToTimePoint(added_at);

  // TMDb/IMDb integration fields
  item.tmdb_id = sqlite3_column_int(stmt, 11);
  if (const char *imdb_id =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 12))) {
    item.imdb_id = imdb_id;
  }
  item.tmdb_rating = sqlite3_column_double(stmt, 13);
  if (const char *trailer_key =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 14))) {
    item.trailer_key = trailer_key;
  }

  // Edition & bonus features fields
  if (const char *edition_type =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 15))) {
    item.edition_type = edition_type;
  }
  item.has_slipcover = sqlite3_column_int(stmt, 16) != 0;
  item.has_digital_copy = sqlite3_column_int(stmt, 17) != 0;
  if (const char *bonus_features =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 18))) {
    item.bonus_features = bonus_features;
  }

  return item;
}

std::string SqliteCollectionRepository::timePointToString(
    const std::chrono::system_clock::time_point &tp) {
  const auto time_t = std::chrono::system_clock::to_time_t(tp);
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

std::chrono::system_clock::time_point
SqliteCollectionRepository::stringToTimePoint(const std::string &str) {
  std::tm tm = {};
  std::istringstream iss(str);
  iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

  const auto time_t = std::mktime(&tm);
  return std::chrono::system_clock::from_time_t(time_t);
}

} // namespace bluray::infrastructure::repositories
