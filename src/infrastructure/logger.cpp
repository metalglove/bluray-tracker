#include "logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace bluray::infrastructure {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(std::string_view log_file_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return;
    }

    log_file_.open(std::string(log_file_path), std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << std::format("Failed to open log file: {}\n", log_file_path);
    }

    initialized_ = true;
    log_impl(LogLevel::Info, "Logger initialized");
}

void Logger::setLevel(LogLevel level) {
    min_level_ = level;
}

void Logger::debug(std::string_view message, const std::source_location& location) {
    if (LogLevel::Debug < min_level_) {
        return;
    }
    log_impl(LogLevel::Debug, message, &location);
}

void Logger::info(std::string_view message, const std::source_location& location) {
    if (LogLevel::Info < min_level_) {
        return;
    }
    log_impl(LogLevel::Info, message, &location);
}

void Logger::warning(std::string_view message, const std::source_location& location) {
    if (LogLevel::Warning < min_level_) {
        return;
    }
    log_impl(LogLevel::Warning, message, &location);
}

void Logger::error(std::string_view message, const std::source_location& location) {
    if (LogLevel::Error < min_level_) {
        return;
    }
    log_impl(LogLevel::Error, message, &location);
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_file_.is_open()) {
        log_impl(LogLevel::Info, "Logger shutting down");
        log_file_.close();
    }
}

Logger::~Logger() {
    close();
}

void Logger::log_impl(LogLevel level, std::string_view message,
                      const std::source_location* location) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string log_entry;

    if (location) {
        log_entry = std::format("[{}] [{}] [{}:{}] {}\n",
            getCurrentTimestamp(),
            levelToString(level),
            location->file_name(),
            location->line(),
            message
        );
    } else {
        log_entry = std::format("[{}] [{}] {}\n",
            getCurrentTimestamp(),
            levelToString(level),
            message
        );
    }

    // Write to console
    std::cout << log_entry;

    // Write to file
    if (log_file_.is_open()) {
        log_file_ << log_entry;
        log_file_.flush();
    }
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() const {
    const auto now = std::chrono::system_clock::now();
    const auto time_t = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}

} // namespace bluray::infrastructure
