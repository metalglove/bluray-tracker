#pragma once

#include "../../domain/models.hpp"
#include <vector>
#include <optional>
#include <memory>
#include <sqlite3.h>

namespace bluray::infrastructure::repositories {

/**
 * Repository interface for collection operations
 */
class ICollectionRepository {
public:
    virtual ~ICollectionRepository() = default;

    virtual int add(const domain::CollectionItem& item) = 0;
    virtual bool update(const domain::CollectionItem& item) = 0;
    virtual bool remove(int id) = 0;
    virtual std::optional<domain::CollectionItem> findById(int id) = 0;
    virtual std::optional<domain::CollectionItem> findByUrl(std::string_view url) = 0;
    virtual std::vector<domain::CollectionItem> findAll() = 0;
    virtual domain::PaginatedResult<domain::CollectionItem> findAll(
        const domain::PaginationParams& params
    ) = 0;
    virtual int count() = 0;
};

/**
 * SQLite implementation of collection repository
 */
class SqliteCollectionRepository : public ICollectionRepository {
public:
    int add(const domain::CollectionItem& item) override;
    bool update(const domain::CollectionItem& item) override;
    bool remove(int id) override;
    std::optional<domain::CollectionItem> findById(int id) override;
    std::optional<domain::CollectionItem> findByUrl(std::string_view url) override;
    std::vector<domain::CollectionItem> findAll() override;
    domain::PaginatedResult<domain::CollectionItem> findAll(
        const domain::PaginationParams& params
    ) override;
    int count() override;

private:
    static domain::CollectionItem fromStatement(sqlite3_stmt* stmt);
    static std::string timePointToString(const std::chrono::system_clock::time_point& tp);
    static std::chrono::system_clock::time_point stringToTimePoint(const std::string& str);
};

} // namespace bluray::infrastructure::repositories
