#include "application/notifier/discord_notifier.hpp"
#include "application/notifier/email_notifier.hpp"
#include "application/scheduler.hpp"
#include "infrastructure/config_manager.hpp"
#include "infrastructure/database_manager.hpp"
#include "infrastructure/logger.hpp"
#include "infrastructure/repositories/release_calendar_repository.hpp"
#include "presentation/web_frontend.hpp"
#include <csignal>
#include <cstring>
#include <fmt/format.h> // Add fmt include
#include <iostream>
#include <memory>
#include <thread>

using namespace bluray;

// Global pointer for signal handling
presentation::WebFrontend *g_web_frontend = nullptr;

void signalHandler(int signum) {
  infrastructure::Logger::instance().info(
      fmt::format("Received signal {}, shutting down...", signum));

  if (g_web_frontend) {
    g_web_frontend->stop();
  }

  std::exit(signum);
}

void printUsage(const char *program_name) {
  std::cout << "Usage: " << program_name << " [OPTIONS]\n\n"
            << "Options:\n"
            << "  --run                Run web server (default mode)\n"
            << "  --scrape             Run wishlist scraper once and exit\n"
            << "  --scrape-calendar    Run release calendar scraper once and exit\n"
            << "  --port <port>        Specify web server port (default: 8080)\n"
            << "  --db <path>          Specify database path (default: "
               "./bluray-tracker.db)\n"
            << "  --help               Show this help message\n"
            << std::endl;
}

int main(int argc, char *argv[]) {
  // Parse command line arguments
  std::string mode = "run";
  int port = 8080;
  std::string db_path = "./bluray-tracker.db";

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--help") == 0) {
      printUsage(argv[0]);
      return 0;
    } else if (std::strcmp(argv[i], "--run") == 0) {
      mode = "run";
    } else if (std::strcmp(argv[i], "--scrape") == 0) {
      mode = "scrape";
    } else if (std::strcmp(argv[i], "--scrape-calendar") == 0) {
      mode = "scrape-calendar";
    } else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
      port = std::stoi(argv[++i]);
    } else if (std::strcmp(argv[i], "--db") == 0 && i + 1 < argc) {
      db_path = argv[++i];
    } else {
      std::cerr << "Unknown option: " << argv[i] << std::endl;
      printUsage(argv[0]);
      return 1;
    }
  }

  try {
    // Initialize infrastructure
    auto &logger = infrastructure::Logger::instance();
    logger.initialize("./bluray-tracker.log");
    logger.setLevel(infrastructure::LogLevel::Info);

    logger.info("=== Blu-ray Tracker Starting ===");

    // Initialize database
    auto &db = infrastructure::DatabaseManager::instance();
    db.initialize(db_path);

    // Load configuration
    auto &config = infrastructure::ConfigManager::instance();
    config.load();

    // Override port from config if not specified via CLI
    if (argc == 1) { // No CLI args
      port = config.getInt("web_port", 8080);
    }

    if (mode == "scrape") {
      // Scrape mode: run once and exit
      logger.info("Running in scrape mode");

      auto scheduler = std::make_shared<application::Scheduler>();

      // Add notifiers
      auto discord_notifier =
          std::make_shared<application::notifier::DiscordNotifier>();
      scheduler->addNotifier(discord_notifier);

      auto email_notifier =
          std::make_shared<application::notifier::EmailNotifier>();
      scheduler->addNotifier(email_notifier);

      // Run scraping
      int processed = scheduler->runOnce();
      logger.info(
          fmt::format("Scraping completed: {} items processed", processed));

      return 0;

    } else if (mode == "scrape-calendar") {
      // Release calendar scrape mode: run once and exit
      logger.info("Running in release calendar scrape mode");

      auto scheduler = std::make_shared<application::Scheduler>();

      // Run calendar scraping
      int processed = scheduler->scrapeReleaseCalendar();
      logger.info(fmt::format("Release calendar scraping completed: {} items processed", processed));

      return 0;

    } else {
      // Web server mode
      logger.info(fmt::format("Running in web server mode on port {}", port));

      // Setup signal handlers
      std::signal(SIGINT, signalHandler);
      std::signal(SIGTERM, signalHandler);

      // Create scheduler
      auto scheduler = std::make_shared<application::Scheduler>();

      // Add notifiers
      auto discord_notifier =
          std::make_shared<application::notifier::DiscordNotifier>();
      scheduler->addNotifier(discord_notifier);

      auto email_notifier =
          std::make_shared<application::notifier::EmailNotifier>();
      scheduler->addNotifier(email_notifier);

      // Check if release calendar is empty and populate in background on first startup
      // This prevents blocking the web server startup while scraping calendar data
      std::thread calendar_init_thread([scheduler]() {
        infrastructure::repositories::SqliteReleaseCalendarRepository calendar_repo;
        int calendar_count = calendar_repo.count();
        auto& logger = infrastructure::Logger::instance();

        if (calendar_count == 0) {
          logger.info("Release calendar is empty, fetching initial data in background...");
          try {
            int processed = scheduler->scrapeReleaseCalendar();
            logger.info(fmt::format(
                "Initial calendar fetch completed: {} releases added", processed));
          } catch (const std::exception &e) {
            logger.warning(fmt::format(
                "Failed to fetch initial calendar data: {}. Will retry on next scheduled run.",
                e.what()));
          }
        } else {
          logger.info(fmt::format(
              "Release calendar already populated with {} items", calendar_count));
        }
      });
      calendar_init_thread.detach();

      // Create and run web frontend
      presentation::WebFrontend web_frontend(scheduler);
      g_web_frontend = &web_frontend;

      logger.info(
          fmt::format("Web interface available at http://localhost:{}", port));
      web_frontend.run(port);
    }

  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
