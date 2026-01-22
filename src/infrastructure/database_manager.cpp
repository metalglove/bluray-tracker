#include "database_manager.hpp"
#include "logger.hpp"
#include <fmt/format.h>

namespace bluray::infrastructure {

DatabaseManager &DatabaseManager::instance() {
  static DatabaseManager instance;
  return instance;
}

void DatabaseManager::initialize(std::string_view db_path) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  if (initialized_) {
    return;
  }

  const int result = sqlite3_open(std::string(db_path).c_str(), &db_);
  if (result != SQLITE_OK) {
    const std::string error =
        fmt::format("Failed to open database: {}", sqlite3_errmsg(db_));
    Logger::instance().error(error);
    throw DatabaseException(error);
  }

  // Enable foreign keys
  execute("PRAGMA foreign_keys = ON");

  // Create schema
  createSchema();
  insertDefaultConfig();

  initialized_ = true;
  Logger::instance().info(fmt::format("Database initialized: {}", db_path));
}

sqlite3 *DatabaseManager::getHandle() { return db_; }

void DatabaseManager::execute(std::string_view sql) {
  char *error_msg = nullptr;
  const int result =
      sqlite3_exec(db_, std::string(sql).c_str(), nullptr, nullptr, &error_msg);

  if (result != SQLITE_OK) {
    std::string error = fmt::format("SQL execution failed: {}",
                                    error_msg ? error_msg : "unknown error");
    if (error_msg) {
      sqlite3_free(error_msg);
    }
    throw DatabaseException(error);
  }
}

Statement DatabaseManager::prepare(std::string_view sql) {
  sqlite3_stmt *stmt = nullptr;
  const int result =
      sqlite3_prepare_v2(db_, std::string(sql).c_str(), -1, &stmt, nullptr);

  if (result != SQLITE_OK) {
    const std::string error =
        fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_));
    throw DatabaseException(error);
  }

  return Statement(stmt);
}

void DatabaseManager::beginTransaction() { execute("BEGIN TRANSACTION"); }

void DatabaseManager::commit() { execute("COMMIT"); }

void DatabaseManager::rollback() { execute("ROLLBACK"); }

int64_t DatabaseManager::lastInsertRowId() const {
  return sqlite3_last_insert_rowid(db_);
}

std::unique_lock<std::recursive_mutex> DatabaseManager::lock() {
  return std::unique_lock<std::recursive_mutex>(mutex_);
}

void DatabaseManager::close() {
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
    Logger::instance().info("Database closed");
  }
}

DatabaseManager::~DatabaseManager() { close(); }

void DatabaseManager::createSchema() {
  // Config table
  execute(R"(
        CREATE TABLE IF NOT EXISTS config (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        )
    )");

  // Wishlist table
  execute(R"(
        CREATE TABLE IF NOT EXISTS wishlist (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            url TEXT NOT NULL UNIQUE,
            title TEXT NOT NULL,
            title_locked INTEGER NOT NULL DEFAULT 0,
            current_price REAL NOT NULL DEFAULT 0.0,
            desired_max_price REAL NOT NULL,
            in_stock INTEGER NOT NULL DEFAULT 0,
            is_uhd_4k INTEGER NOT NULL DEFAULT 0,
            image_url TEXT,
            local_image_path TEXT,
            source TEXT NOT NULL,
            notify_on_price_drop INTEGER NOT NULL DEFAULT 1,
            notify_on_stock INTEGER NOT NULL DEFAULT 1,
            created_at TEXT NOT NULL,
            last_checked TEXT NOT NULL
        )
    )");

  // Collection table
  execute(R"(
        CREATE TABLE IF NOT EXISTS collection (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            url TEXT NOT NULL UNIQUE,
            title TEXT NOT NULL,
            purchase_price REAL NOT NULL DEFAULT 0.0,
            is_uhd_4k INTEGER NOT NULL DEFAULT 0,
            image_url TEXT,
            local_image_path TEXT,
            source TEXT NOT NULL,
            notes TEXT,
            purchased_at TEXT NOT NULL,
            added_at TEXT NOT NULL
        )
    )");

  // Price history table (for tracking changes over time)
  execute(R"(
        CREATE TABLE IF NOT EXISTS price_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            wishlist_id INTEGER NOT NULL,
            price REAL NOT NULL,
            in_stock INTEGER NOT NULL,
            recorded_at TEXT NOT NULL,
            FOREIGN KEY (wishlist_id) REFERENCES wishlist(id) ON DELETE CASCADE
        )
    )");

  // Release calendar table
  execute(R"(
        CREATE TABLE IF NOT EXISTS release_calendar (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            release_date TEXT NOT NULL,
            format TEXT NOT NULL,
            studio TEXT,
            image_url TEXT,
            local_image_path TEXT,
            product_url TEXT,
            is_uhd_4k INTEGER NOT NULL DEFAULT 0,
            is_preorder INTEGER NOT NULL DEFAULT 0,
            price REAL,
            notes TEXT,
            created_at TEXT NOT NULL,
            last_updated TEXT NOT NULL
        )
    )");

  // Create indices for better performance
  execute("CREATE INDEX IF NOT EXISTS idx_wishlist_url ON wishlist(url)");
  execute("CREATE INDEX IF NOT EXISTS idx_collection_url ON collection(url)");
  execute("CREATE INDEX IF NOT EXISTS idx_price_history_wishlist ON "
          "price_history(wishlist_id)");
  execute("CREATE INDEX IF NOT EXISTS idx_release_calendar_date ON "
          "release_calendar(release_date)");
  execute("CREATE INDEX IF NOT EXISTS idx_release_calendar_url ON "
          "release_calendar(product_url)");

  // Tags table for custom labels
  execute(R"(
        CREATE TABLE IF NOT EXISTS tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            color TEXT NOT NULL DEFAULT '#667eea'
        )
    )");

  // Item-tag mappings
  execute(R"(
        CREATE TABLE IF NOT EXISTS item_tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            item_id INTEGER NOT NULL,
            item_type TEXT NOT NULL,
            tag_id INTEGER NOT NULL,
            FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE,
            UNIQUE(item_id, item_type, tag_id)
        )
    )");

  // Index for tag lookups
  execute("CREATE INDEX IF NOT EXISTS idx_item_tags_item ON "
          "item_tags(item_id, item_type)");
  execute("CREATE INDEX IF NOT EXISTS idx_item_tags_tag ON "
          "item_tags(tag_id)");

  // Migrations
  try {
    execute("ALTER TABLE wishlist ADD COLUMN title_locked INTEGER NOT NULL "
            "DEFAULT 0");
  } catch (...) {
    // Column likely exists
  }

  // TMDb/IMDb integration columns for wishlist
  try {
    execute("ALTER TABLE wishlist ADD COLUMN tmdb_id INTEGER DEFAULT 0");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE wishlist ADD COLUMN imdb_id TEXT DEFAULT ''");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE wishlist ADD COLUMN tmdb_rating REAL DEFAULT 0.0");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE wishlist ADD COLUMN trailer_key TEXT DEFAULT ''");
  } catch (...) {
  }

  // Edition & bonus features columns for wishlist
  try {
    execute("ALTER TABLE wishlist ADD COLUMN edition_type TEXT DEFAULT ''");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE wishlist ADD COLUMN has_slipcover INTEGER DEFAULT 0");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE wishlist ADD COLUMN has_digital_copy INTEGER DEFAULT 0");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE wishlist ADD COLUMN bonus_features TEXT DEFAULT ''");
  } catch (...) {
  }

  // TMDb/IMDb integration columns for collection
  try {
    execute("ALTER TABLE collection ADD COLUMN tmdb_id INTEGER DEFAULT 0");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE collection ADD COLUMN imdb_id TEXT DEFAULT ''");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE collection ADD COLUMN tmdb_rating REAL DEFAULT 0.0");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE collection ADD COLUMN trailer_key TEXT DEFAULT ''");
  } catch (...) {
  }

  // Edition & bonus features columns for collection
  try {
    execute("ALTER TABLE collection ADD COLUMN edition_type TEXT DEFAULT ''");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE collection ADD COLUMN has_slipcover INTEGER DEFAULT 0");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE collection ADD COLUMN has_digital_copy INTEGER DEFAULT 0");
  } catch (...) {
  }
  try {
    execute("ALTER TABLE collection ADD COLUMN bonus_features TEXT DEFAULT ''");
  } catch (...) {
  }
}

void DatabaseManager::insertDefaultConfig() {
  // Check if config already exists
  auto stmt = prepare("SELECT COUNT(*) FROM config");
  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    const int count = sqlite3_column_int(stmt.get(), 0);
    if (count > 0) {
      return; // Config already initialized
    }
  }

  // Insert default configuration
  execute(R"(
        INSERT OR IGNORE INTO config (key, value) VALUES
        ('scrape_delay_seconds', '8'),
        ('discord_webhook_url', ''),
        ('smtp_server', ''),
        ('smtp_port', '587'),
        ('smtp_user', ''),
        ('smtp_pass', ''),
        ('smtp_from', ''),
        ('smtp_to', ''),
        ('web_port', '8080'),
        ('cache_directory', './cache'),
        ('log_file', './bluray-tracker.log'),
        ('log_level', 'info'),
        ('bluray_calendar_url', 'https://www.blu-ray.com/movies/releasedates.php'),
        ('bluray_calendar_enabled', '1'),
        ('bluray_calendar_days_ahead', '90')
    )");

  Logger::instance().info("Default configuration inserted");
}

} // namespace bluray::infrastructure
