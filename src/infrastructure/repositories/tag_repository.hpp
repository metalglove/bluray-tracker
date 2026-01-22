#pragma once

#include "../../domain/models.hpp"
#include <optional>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace bluray::infrastructure::repositories {

/**
 * Repository interface for tag operations
 */
class ITagRepository {
public:
  virtual ~ITagRepository() = default;

  virtual int add(const domain::Tag &tag) = 0;
  virtual bool update(const domain::Tag &tag) = 0;
  virtual bool remove(int id) = 0;
  virtual std::optional<domain::Tag> findById(int id) = 0;
  virtual std::optional<domain::Tag> findByName(std::string_view name) = 0;
  virtual std::vector<domain::Tag> findAll() = 0;

  // Tag-item associations
  virtual bool addTagToItem(int tag_id, int item_id,
                            const std::string &item_type) = 0;
  virtual bool removeTagFromItem(int tag_id, int item_id,
                                 const std::string &item_type) = 0;
  virtual std::vector<domain::Tag> getTagsForItem(int item_id,
                                                  const std::string &item_type) = 0;
  virtual std::vector<int> getItemIdsForTag(int tag_id,
                                            const std::string &item_type) = 0;
};

/**
 * SQLite implementation of tag repository
 */
class SqliteTagRepository : public ITagRepository {
public:
  int add(const domain::Tag &tag) override;
  bool update(const domain::Tag &tag) override;
  bool remove(int id) override;
  std::optional<domain::Tag> findById(int id) override;
  std::optional<domain::Tag> findByName(std::string_view name) override;
  std::vector<domain::Tag> findAll() override;

  bool addTagToItem(int tag_id, int item_id,
                    const std::string &item_type) override;
  bool removeTagFromItem(int tag_id, int item_id,
                         const std::string &item_type) override;
  std::vector<domain::Tag> getTagsForItem(int item_id,
                                          const std::string &item_type) override;
  std::vector<int> getItemIdsForTag(int tag_id,
                                    const std::string &item_type) override;

private:
  static domain::Tag fromStatement(sqlite3_stmt *stmt);
};

} // namespace bluray::infrastructure::repositories
