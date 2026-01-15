#pragma once

#include <sqlite3.h>
#include <string>
#include <string_view>
#include <mutex>
#include <memory>
#include <stdexcept>

namespace bluray::infrastructure {

/**
 * RAII wrapper for SQLite statement
 */
class Statement {
public:
    explicit Statement(sqlite3_stmt* stmt) : stmt_(stmt) {}

    ~Statement() {
        if (stmt_) {
            sqlite3_finalize(stmt_);
        }
    }

    // Prevent copying
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    // Allow moving
    Statement(Statement&& other) noexcept : stmt_(other.stmt_) {
        other.stmt_ = nullptr;
    }

    Statement& operator=(Statement&& other) noexcept {
        if (this != &other) {
            if (stmt_) {
                sqlite3_finalize(stmt_);
            }
            stmt_ = other.stmt_;
            other.stmt_ = nullptr;
        }
        return *this;
    }

    sqlite3_stmt* get() const { return stmt_; }
    operator sqlite3_stmt*() const { return stmt_; }

private:
    sqlite3_stmt* stmt_;
};

/**
 * Database exception
 */
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * Singleton database manager with RAII SQLite connection
 */
class DatabaseManager {
public:
    /**
     * Get singleton instance
     */
    static DatabaseManager& instance();

    /**
     * Initialize database connection and create schema
     */
    void initialize(std::string_view db_path);

    /**
     * Get raw database handle (for advanced operations)
     * Thread-safe: caller must hold lock
     */
    sqlite3* getHandle();

    /**
     * Execute a SQL statement without results
     */
    void execute(std::string_view sql);

    /**
     * Prepare a SQL statement
     */
    [[nodiscard]] Statement prepare(std::string_view sql);

    /**
     * Begin transaction
     */
    void beginTransaction();

    /**
     * Commit transaction
     */
    void commit();

    /**
     * Rollback transaction
     */
    void rollback();

    /**
     * Get last insert row ID
     */
    [[nodiscard]] int64_t lastInsertRowId() const;

    /**
     * Lock for thread-safe operations
     */
    std::unique_lock<std::mutex> lock();

    /**
     * Close database connection
     */
    void close();

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    // Prevent copying
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    void createSchema();
    void insertDefaultConfig();

    sqlite3* db_{nullptr};
    std::mutex mutex_;
    bool initialized_{false};
};

/**
 * RAII transaction guard
 */
class Transaction {
public:
    explicit Transaction(DatabaseManager& db) : db_(db), committed_(false) {
        db_.beginTransaction();
    }

    ~Transaction() {
        if (!committed_) {
            try {
                db_.rollback();
            } catch (...) {
                // Ignore exceptions in destructor
            }
        }
    }

    void commit() {
        db_.commit();
        committed_ = true;
    }

private:
    DatabaseManager& db_;
    bool committed_;
};

} // namespace bluray::infrastructure
