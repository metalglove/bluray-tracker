#pragma once

#include "../../domain/models.hpp"
#include <vector>
#include <optional>
#include <memory>
#include <sqlite3.h>

namespace bluray::infrastructure::repositories {

/**
 * Repository interface for release calendar operations
 */
class IReleaseCalendarRepository {
public:
    virtual ~IReleaseCalendarRepository() = default;

    virtual int add(const domain::ReleaseCalendarItem& item) = 0;
    virtual bool update(const domain::ReleaseCalendarItem& item) = 0;
    virtual bool remove(int id) = 0;
    virtual std::optional<domain::ReleaseCalendarItem> findById(int id) = 0;
    virtual std::optional<domain::ReleaseCalendarItem> findByUrl(std::string_view url) = 0;
    virtual std::vector<domain::ReleaseCalendarItem> findAll() = 0;
    virtual domain::PaginatedResult<domain::ReleaseCalendarItem> findAll(
        const domain::PaginationParams& params
    ) = 0;
    virtual std::vector<domain::ReleaseCalendarItem> findByDateRange(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end
    ) = 0;
    virtual int count() = 0;
    virtual int removeOlderThan(const std::chrono::system_clock::time_point& cutoff_date) = 0;
};

/**
 * SQLite implementation of release calendar repository
 */
class SqliteReleaseCalendarRepository : public IReleaseCalendarRepository {
public:
    int add(const domain::ReleaseCalendarItem& item) override;
    bool update(const domain::ReleaseCalendarItem& item) override;
    bool remove(int id) override;
    std::optional<domain::ReleaseCalendarItem> findById(int id) override;
    std::optional<domain::ReleaseCalendarItem> findByUrl(std::string_view url) override;
    std::vector<domain::ReleaseCalendarItem> findAll() override;
    domain::PaginatedResult<domain::ReleaseCalendarItem> findAll(
        const domain::PaginationParams& params
    ) override;
    std::vector<domain::ReleaseCalendarItem> findByDateRange(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end
    ) override;
    int count() override;
    int removeOlderThan(const std::chrono::system_clock::time_point& cutoff_date) override;

private:
    static domain::ReleaseCalendarItem fromStatement(sqlite3_stmt* stmt);
    static std::string timePointToString(const std::chrono::system_clock::time_point& tp);
    static std::chrono::system_clock::time_point stringToTimePoint(const std::string& str);
};

} // namespace bluray::infrastructure::repositories
