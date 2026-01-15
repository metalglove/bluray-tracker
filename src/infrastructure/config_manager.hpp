#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <mutex>

namespace bluray::infrastructure {

/**
 * Configuration manager that stores settings in SQLite
 * Thread-safe singleton
 */
class ConfigManager {
public:
    /**
     * Get singleton instance
     */
    static ConfigManager& instance();

    /**
     * Load configuration from database
     */
    void load();

    /**
     * Get configuration value
     */
    [[nodiscard]] std::optional<std::string> get(std::string_view key) const;

    /**
     * Get configuration value with default
     */
    [[nodiscard]] std::string get(std::string_view key, std::string_view default_value) const;

    /**
     * Get configuration value as integer
     */
    [[nodiscard]] int getInt(std::string_view key, int default_value = 0) const;

    /**
     * Get configuration value as double
     */
    [[nodiscard]] double getDouble(std::string_view key, double default_value = 0.0) const;

    /**
     * Get configuration value as boolean
     */
    [[nodiscard]] bool getBool(std::string_view key, bool default_value = false) const;

    /**
     * Set configuration value
     */
    void set(std::string_view key, std::string_view value);

    /**
     * Set configuration value (integer)
     */
    void set(std::string_view key, int value);

    /**
     * Set configuration value (double)
     */
    void set(std::string_view key, double value);

    /**
     * Set configuration value (boolean)
     */
    void set(std::string_view key, bool value);

    /**
     * Check if key exists
     */
    [[nodiscard]] bool has(std::string_view key) const;

    /**
     * Reload configuration from database
     */
    void reload();

private:
    ConfigManager() = default;

    // Prevent copying
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    void loadFromDatabase();

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> config_;
    bool loaded_{false};
};

} // namespace bluray::infrastructure
