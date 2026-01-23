#include "deal_repository.hpp"
#include "../logger.hpp"

#include <fmt/format.h>
#include <iomanip>
#include <sstream>

namespace bluray::infrastructure {

int64_t DealRepository::add(const domain::Deal &deal) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  std::string ends_at_str =
      deal.ends_at ? timePointToString(*deal.ends_at) : "";

  auto stmt = db.prepare(R"(
        INSERT INTO deals (url, title, source, original_price, deal_price,
                          discount_percentage, deal_type, ends_at, is_uhd_4k,
                          image_url, local_image_path, discovered_at, last_checked, is_active)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

  sqlite3_bind_text(stmt.get(), 1, deal.url.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, deal.title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, deal.source.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt.get(), 4, deal.original_price);
  sqlite3_bind_double(stmt.get(), 5, deal.deal_price);
  sqlite3_bind_double(stmt.get(), 6, deal.discount_percentage);
  sqlite3_bind_text(stmt.get(), 7, deal.deal_type.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 8, ends_at_str.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 9, deal.is_uhd_4k ? 1 : 0);
  sqlite3_bind_text(stmt.get(), 10, deal.image_url.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 11, deal.local_image_path.c_str(), -1, SQLITE_TRANSIENT);
  
  std::string discovered_at_str = timePointToString(deal.discovered_at);
  std::string last_checked_str = timePointToString(deal.last_checked);
  sqlite3_bind_text(stmt.get(), 12, discovered_at_str.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 13, last_checked_str.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 14, deal.is_active ? 1 : 0);

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    Logger::instance().error(
        fmt::format("Failed to add deal: {}", deal.title));
    return 0;
  }

  return db.lastInsertRowId();
}

bool DealRepository::update(const domain::Deal &deal) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  std::string ends_at_str =
      deal.ends_at ? timePointToString(*deal.ends_at) : "";

  auto stmt = db.prepare(R"(
        UPDATE deals
        SET title = ?, original_price = ?, deal_price = ?,
            discount_percentage = ?, deal_type = ?, ends_at = ?,
            image_url = ?, local_image_path = ?, last_checked = ?,
            is_active = ?
        WHERE id = ?
    )");

  sqlite3_bind_text(stmt.get(), 1, deal.title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt.get(), 2, deal.original_price);
  sqlite3_bind_double(stmt.get(), 3, deal.deal_price);
  sqlite3_bind_double(stmt.get(), 4, deal.discount_percentage);
  sqlite3_bind_text(stmt.get(), 5, deal.deal_type.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 6, ends_at_str.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 7, deal.image_url.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 8, deal.local_image_path.c_str(), -1, SQLITE_TRANSIENT);
  
  std::string last_checked_str2 = timePointToString(deal.last_checked);
  sqlite3_bind_text(stmt.get(), 9, last_checked_str2.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 10, deal.is_active ? 1 : 0);
  sqlite3_bind_int64(stmt.get(), 11, deal.id);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

std::optional<domain::Deal> DealRepository::findById(int64_t id) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT * FROM deals WHERE id = ?");
  sqlite3_bind_int64(stmt.get(), 1, id);

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return dealFromStatement(stmt);
  }

  return std::nullopt;
}

std::optional<domain::Deal>
DealRepository::findByUrl(std::string_view url) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT * FROM deals WHERE url = ?");
  sqlite3_bind_text(stmt.get(), 1, url.data(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return dealFromStatement(stmt);
  }

  return std::nullopt;
}

std::vector<domain::Deal> DealRepository::getAllActive() {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(
      "SELECT * FROM deals WHERE is_active = 1 ORDER BY discount_percentage "
      "DESC, discovered_at DESC");

  std::vector<domain::Deal> deals;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    deals.push_back(dealFromStatement(stmt));
  }

  return deals;
}

std::vector<domain::Deal> DealRepository::getActive(int page, int page_size) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  const int offset = (page - 1) * page_size;

  auto stmt = db.prepare(R"(
        SELECT * FROM deals
        WHERE is_active = 1
        ORDER BY discount_percentage DESC, discovered_at DESC
        LIMIT ? OFFSET ?
    )");

  sqlite3_bind_int(stmt.get(), 1, page_size);
  sqlite3_bind_int(stmt.get(), 2, offset);

  std::vector<domain::Deal> deals;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    deals.push_back(dealFromStatement(stmt));
  }

  return deals;
}

int DealRepository::getActiveCount() {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT COUNT(*) FROM deals WHERE is_active = 1");

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return sqlite3_column_int(stmt.get(), 0);
  }

  return 0;
}

std::vector<domain::Deal>
DealRepository::getFiltered(bool only_4k, std::string_view source,
                            double min_discount, int page, int page_size) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  std::string sql = "SELECT * FROM deals WHERE is_active = 1";

  if (only_4k) {
    sql += " AND is_uhd_4k = 1";
  }

  if (!source.empty()) {
    sql += " AND source = ?";
  }

  if (min_discount > 0) {
    sql += " AND discount_percentage >= ?";
  }

  sql += " ORDER BY discount_percentage DESC, discovered_at DESC LIMIT ? "
         "OFFSET ?";

  auto stmt = db.prepare(sql);

  int param_index = 1;
  if (!source.empty()) {
    sqlite3_bind_text(stmt.get(), param_index++, source.data(), source.size(), SQLITE_TRANSIENT);
  }
  if (min_discount > 0) {
    sqlite3_bind_double(stmt.get(), param_index++, min_discount);
  }

  const int offset = (page - 1) * page_size;
  sqlite3_bind_int(stmt.get(), param_index++, page_size);
  sqlite3_bind_int(stmt.get(), param_index++, offset);

  std::vector<domain::Deal> deals;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    deals.push_back(dealFromStatement(stmt));
  }

  return deals;
}

int DealRepository::getFilteredCount(bool only_4k, std::string_view source,
                                     double min_discount) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  std::string sql = "SELECT COUNT(*) FROM deals WHERE is_active = 1";

  if (only_4k) {
    sql += " AND is_uhd_4k = 1";
  }

  if (!source.empty()) {
    sql += " AND source = ?";
  }

  if (min_discount > 0) {
    sql += " AND discount_percentage >= ?";
  }

  auto stmt = db.prepare(sql);

  int param_index = 1;
  if (!source.empty()) {
    sqlite3_bind_text(stmt.get(), param_index++, source.data(), source.size(), SQLITE_TRANSIENT);
  }
  if (min_discount > 0) {
    sqlite3_bind_double(stmt.get(), param_index++, min_discount);
  }

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return sqlite3_column_int(stmt.get(), 0);
  }

  return 0;
}

bool DealRepository::markInactive(int64_t id) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("UPDATE deals SET is_active = 0 WHERE id = ?");
  sqlite3_bind_int64(stmt.get(), 1, id);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

bool DealRepository::remove(int64_t id) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("DELETE FROM deals WHERE id = ?");
  sqlite3_bind_int64(stmt.get(), 1, id);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

int DealRepository::markExpiredInactive() {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto now_str = timePointToString(std::chrono::system_clock::now());

  auto stmt = db.prepare(
      "UPDATE deals SET is_active = 0 WHERE is_active = 1 AND ends_at != '' "
      "AND ends_at < ?");
  sqlite3_bind_text(stmt.get(), 1, now_str.c_str(), -1, SQLITE_TRANSIENT);

  sqlite3_step(stmt.get());

  return sqlite3_changes(db.getHandle());
}

domain::Deal DealRepository::dealFromStatement(Statement &stmt) {
  domain::Deal deal;

  deal.id = sqlite3_column_int64(stmt.get(), 0);
  deal.url = reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 1));
  deal.title =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 2));
  deal.source =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 3));

  deal.original_price = sqlite3_column_double(stmt.get(), 4);
  deal.deal_price = sqlite3_column_double(stmt.get(), 5);
  deal.discount_percentage = sqlite3_column_double(stmt.get(), 6);

  deal.deal_type =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 7));

  const char *ends_at_str =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 8));
  if (ends_at_str && std::strlen(ends_at_str) > 0) {
    deal.ends_at = stringToTimePoint(ends_at_str);
  }

  deal.is_uhd_4k = sqlite3_column_int(stmt.get(), 9) == 1;

  const char *img_url =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 10));
  deal.image_url = img_url ? img_url : "";

  const char *local_img =
      reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 11));
  deal.local_image_path = local_img ? local_img : "";

  deal.discovered_at = stringToTimePoint(reinterpret_cast<const char *>(
      sqlite3_column_text(stmt.get(), 12)));
  deal.last_checked = stringToTimePoint(reinterpret_cast<const char *>(
      sqlite3_column_text(stmt.get(), 13)));

  deal.is_active = sqlite3_column_int(stmt.get(), 14) == 1;

  return deal;
}

std::string DealRepository::timePointToString(
    const std::chrono::system_clock::time_point &tp) {
  const std::time_t time = std::chrono::system_clock::to_time_t(tp);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

std::chrono::system_clock::time_point
DealRepository::stringToTimePoint(const std::string &str) {
  std::tm tm = {};
  std::stringstream ss(str);
  ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
  return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

} // namespace bluray::infrastructure

