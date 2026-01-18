#pragma once

#include <chrono>
#include <fmt/format.h>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

namespace bluray::infrastructure {

/**
 * Log levels
 */
enum class LogLevel { Debug, Info, Warning, Error };

/**
 * Thread-safe logger with file and console output
 * Uses modern C++20 features: std::format, std::source_location
 */
class Logger {
public:
  /**
   * Get singleton instance
   */
  static Logger &instance();

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
  /**
   * Log methods
   */
  void debug(std::string_view message);

  void info(std::string_view message);

  void warning(std::string_view message);

  void error(std::string_view message);

  /**
   * Templated log method with formatting
   */
  template <typename... Args>
  void log(LogLevel level, fmt::format_string<Args...> fmt, Args &&...args) {
    if (level < min_level_) {
      return;
    }

    const auto message = fmt::format(fmt, std::forward<Args>(args)...);
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
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  void log_impl(LogLevel level, std::string_view message);

  [[nodiscard]] std::string levelToString(LogLevel level) const;
  [[nodiscard]] std::string getCurrentTimestamp() const;

  std::ofstream log_file_;
  std::recursive_mutex mutex_;
  LogLevel min_level_{LogLevel::Info};
  bool initialized_{false};
};

} // namespace bluray::infrastructure
