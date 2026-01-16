#pragma once

#include "../../domain/models.hpp"
#include <vector>
#include <optional>
#include <memory>
#include <sqlite3.h>

namespace bluray::infrastructure::repositories {

/**
 * Repository interface for wishlist operations
 */
class IWishlistRepository {
public:
    virtual ~IWishlistRepository() = default;

    virtual int add(const domain::WishlistItem& item) = 0;
    virtual bool update(const domain::WishlistItem& item) = 0;
    virtual bool remove(int id) = 0;
    virtual std::optional<domain::WishlistItem> findById(int id) = 0;
    virtual std::optional<domain::WishlistItem> findByUrl(std::string_view url) = 0;
    virtual std::vector<domain::WishlistItem> findAll() = 0;
    virtual domain::PaginatedResult<domain::WishlistItem> findAll(
        const domain::PaginationParams& params
    ) = 0;
    virtual int count() = 0;
};

/**
 * SQLite implementation of wishlist repository
 */
class SqliteWishlistRepository : public IWishlistRepository {
public:
    int add(const domain::WishlistItem& item) override;
    bool update(const domain::WishlistItem& item) override;
    bool remove(int id) override;
    std::optional<domain::WishlistItem> findById(int id) override;
    std::optional<domain::WishlistItem> findByUrl(std::string_view url) override;
    std::vector<domain::WishlistItem> findAll() override;
    domain::PaginatedResult<domain::WishlistItem> findAll(
        const domain::PaginationParams& params
    ) override;
    int count() override;

private:
    static domain::WishlistItem fromStatement(sqlite3_stmt* stmt);
    static std::string timePointToString(const std::chrono::system_clock::time_point& tp);
    static std::chrono::system_clock::time_point stringToTimePoint(const std::string& str);
};

} // namespace bluray::infrastructure::repositories
