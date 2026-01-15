#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <mutex>
#include <source_location>
#include <format>
#include <chrono>

namespace bluray::infrastructure {

/**
 * Log levels
 */
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

/**
 * Thread-safe logger with file and console output
 * Uses modern C++20 features: std::format, std::source_location
 */
class Logger {
public:
    /**
     * Get singleton instance
     */
    static Logger& instance();

    /**
     * Initialize logger with log file path
     */
    void initialize(std::string_view log_file_path);

    /**
     * Set minimum log level
     */
    void setLevel(LogLevel level);

    /**
     * Log methods
     */
    void debug(std::string_view message,
               const std::source_location& location = std::source_location::current());

    void info(std::string_view message,
              const std::source_location& location = std::source_location::current());

    void warning(std::string_view message,
                 const std::source_location& location = std::source_location::current());

    void error(std::string_view message,
               const std::source_location& location = std::source_location::current());

    /**
     * Templated log method with formatting
     */
    template<typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        if (level < min_level_) {
            return;
        }

        const auto message = std::format(fmt, std::forward<Args>(args)...);
        log_impl(level, message);
    }

    /**
     * Close log file
     */
    void close();

private:
    Logger() = default;
    ~Logger();

    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log_impl(LogLevel level, std::string_view message,
                  const std::source_location* location = nullptr);

    [[nodiscard]] std::string levelToString(LogLevel level) const;
    [[nodiscard]] std::string getCurrentTimestamp() const;

    std::ofstream log_file_;
    std::mutex mutex_;
    LogLevel min_level_{LogLevel::Info};
    bool initialized_{false};
};

} // namespace bluray::infrastructure
