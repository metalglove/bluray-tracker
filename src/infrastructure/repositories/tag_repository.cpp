#include "tag_repository.hpp"
#include "../database_manager.hpp"
#include "../logger.hpp"
#include <fmt/format.h>

namespace bluray::infrastructure::repositories {

int SqliteTagRepository::add(const domain::Tag &tag) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        INSERT INTO tags (name, color) VALUES (?, ?)
    )");

  sqlite3_bind_text(stmt.get(), 1, tag.name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, tag.color.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    Logger::instance().error(fmt::format("Failed to insert tag: {}",
                                         sqlite3_errmsg(db.getHandle())));
    return -1;
  }

  return static_cast<int>(db.lastInsertRowId());
}

bool SqliteTagRepository::update(const domain::Tag &tag) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        UPDATE tags SET name = ?, color = ? WHERE id = ?
    )");

  sqlite3_bind_text(stmt.get(), 1, tag.name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, tag.color.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 3, tag.id);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

bool SqliteTagRepository::remove(int id) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  // Tags are deleted via CASCADE on item_tags
  auto stmt = db.prepare("DELETE FROM tags WHERE id = ?");
  sqlite3_bind_int(stmt.get(), 1, id);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

std::optional<domain::Tag> SqliteTagRepository::findById(int id) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT id, name, color FROM tags WHERE id = ?");
  sqlite3_bind_int(stmt.get(), 1, id);

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return fromStatement(stmt.get());
  }

  return std::nullopt;
}

std::optional<domain::Tag>
SqliteTagRepository::findByName(std::string_view name) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT id, name, color FROM tags WHERE name = ?");
  sqlite3_bind_text(stmt.get(), 1, std::string(name).c_str(), -1,
                    SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return fromStatement(stmt.get());
  }

  return std::nullopt;
}

std::vector<domain::Tag> SqliteTagRepository::findAll() {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare("SELECT id, name, color FROM tags ORDER BY name ASC");

  std::vector<domain::Tag> tags;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    tags.push_back(fromStatement(stmt.get()));
  }

  return tags;
}

bool SqliteTagRepository::addTagToItem(int tag_id, int item_id,
                                       const std::string &item_type) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        INSERT OR IGNORE INTO item_tags (tag_id, item_id, item_type)
        VALUES (?, ?, ?)
    )");

  sqlite3_bind_int(stmt.get(), 1, tag_id);
  sqlite3_bind_int(stmt.get(), 2, item_id);
  sqlite3_bind_text(stmt.get(), 3, item_type.c_str(), -1, SQLITE_TRANSIENT);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

bool SqliteTagRepository::removeTagFromItem(int tag_id, int item_id,
                                            const std::string &item_type) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        DELETE FROM item_tags
        WHERE tag_id = ? AND item_id = ? AND item_type = ?
    )");

  sqlite3_bind_int(stmt.get(), 1, tag_id);
  sqlite3_bind_int(stmt.get(), 2, item_id);
  sqlite3_bind_text(stmt.get(), 3, item_type.c_str(), -1, SQLITE_TRANSIENT);

  return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

std::vector<domain::Tag>
SqliteTagRepository::getTagsForItem(int item_id, const std::string &item_type) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        SELECT t.id, t.name, t.color
        FROM tags t
        INNER JOIN item_tags it ON t.id = it.tag_id
        WHERE it.item_id = ? AND it.item_type = ?
        ORDER BY t.name ASC
    )");

  sqlite3_bind_int(stmt.get(), 1, item_id);
  sqlite3_bind_text(stmt.get(), 2, item_type.c_str(), -1, SQLITE_TRANSIENT);

  std::vector<domain::Tag> tags;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    tags.push_back(fromStatement(stmt.get()));
  }

  return tags;
}

std::vector<int>
SqliteTagRepository::getItemIdsForTag(int tag_id, const std::string &item_type) {
  auto &db = DatabaseManager::instance();
  auto lock = db.lock();

  auto stmt = db.prepare(R"(
        SELECT item_id FROM item_tags
        WHERE tag_id = ? AND item_type = ?
    )");

  sqlite3_bind_int(stmt.get(), 1, tag_id);
  sqlite3_bind_text(stmt.get(), 2, item_type.c_str(), -1, SQLITE_TRANSIENT);

  std::vector<int> item_ids;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    item_ids.push_back(sqlite3_column_int(stmt.get(), 0));
  }

  return item_ids;
}

domain::Tag SqliteTagRepository::fromStatement(sqlite3_stmt *stmt) {
  domain::Tag tag;
  tag.id = sqlite3_column_int(stmt, 0);
  tag.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
  if (const char *color =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2))) {
    tag.color = color;
  }
  return tag;
}

} // namespace bluray::infrastructure::repositories
