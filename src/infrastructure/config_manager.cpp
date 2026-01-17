#include "config_manager.hpp"
#include "database_manager.hpp"
#include "logger.hpp"
#include <fmt/format.h>

namespace bluray::infrastructure {

ConfigManager &ConfigManager::instance() {
  static ConfigManager instance;
  return instance;
}

void ConfigManager::load() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (loaded_) {
    return;
  }

  loadFromDatabase();
  loaded_ = true;
  Logger::instance().info("Configuration loaded");
}

std::optional<std::string> ConfigManager::get(std::string_view key) const {
  std::lock_guard<std::mutex> lock(mutex_);

  const auto it = config_.find(std::string(key));
  if (it != config_.end()) {
    return it->second;
  }

  return std::nullopt;
}

std::string ConfigManager::get(std::string_view key,
                               std::string_view default_value) const {
  auto value = get(key);
  return value ? *value : std::string(default_value);
}

int ConfigManager::getInt(std::string_view key, int default_value) const {
  auto value = get(key);
  if (!value) {
    return default_value;
  }

  try {
    return std::stoi(*value);
  } catch (...) {
    Logger::instance().warning(
        fmt::format("Failed to parse int config '{}': {}", key, *value));
    return default_value;
  }
}

double ConfigManager::getDouble(std::string_view key,
                                double default_value) const {
  auto value = get(key);
  if (!value) {
    return default_value;
  }

  try {
    return std::stod(*value);
  } catch (...) {
    Logger::instance().warning(
        fmt::format("Failed to parse double config '{}': {}", key, *value));
    return default_value;
  }
}

bool ConfigManager::getBool(std::string_view key, bool default_value) const {
  auto value = get(key);
  if (!value) {
    return default_value;
  }

  const auto &str = *value;
  return str == "1" || str == "true" || str == "yes" || str == "on";
}

void ConfigManager::set(std::string_view key, std::string_view value) {
  std::lock_guard<std::mutex> lock(mutex_);

  // Update in-memory cache
  config_[std::string(key)] = value;

  // Update in database
  auto &db = DatabaseManager::instance();
  auto db_lock = db.lock();

  auto stmt =
      db.prepare("INSERT OR REPLACE INTO config (key, value) VALUES (?, ?)");

  sqlite3_bind_text(stmt.get(), 1, std::string(key).c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, std::string(value).c_str(), -1,
                    SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    Logger::instance().error(fmt::format("Failed to set config '{}': {}", key,
                                         sqlite3_errmsg(db.getHandle())));
  }
}

void ConfigManager::set(std::string_view key, int value) {
  set(key, std::to_string(value));
}

void ConfigManager::set(std::string_view key, double value) {
  set(key, fmt::format("{:.2f}", value));
}

void ConfigManager::set(std::string_view key, bool value) {
  set(key, std::string(value ? "1" : "0"));
}

bool ConfigManager::has(std::string_view key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return config_.count(std::string(key)) > 0;
}

void ConfigManager::reload() {
  std::lock_guard<std::mutex> lock(mutex_);
  loadFromDatabase();
  Logger::instance().info("Configuration reloaded");
}

void ConfigManager::loadFromDatabase() {
  auto &db = DatabaseManager::instance();
  auto db_lock = db.lock();

  config_.clear();

  auto stmt = db.prepare("SELECT key, value FROM config");

  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    const char *key =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 0));
    const char *value =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 1));

    if (key && value) {
      config_[key] = value;
    }
  }
}

} // namespace bluray::infrastructure
