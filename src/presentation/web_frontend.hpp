#pragma once

#include "../application/scheduler.hpp"
#include "../domain/deal.hpp"

#include "html_renderer.hpp"
#include <crow.h>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace bluray::presentation {

/**
 * Web frontend using Crow framework with WebSocket support
 */
class WebFrontend {
public:
  explicit WebFrontend(std::shared_ptr<application::Scheduler> scheduler);

  /**
   * Destructor - joins background threads
   */
  ~WebFrontend();

  /**
   * Start web server
   */
  void run(int port = 8080);

  /**
   * Stop web server
   */
  void stop();

  /**
   * Clean up finished background threads
   */
  void cleanupFinishedThreads();

  /**
   * Broadcast update to all connected WebSocket clients
   */
  void broadcastUpdate(const std::string &message);

private:
  void setupRoutes();

  // API endpoints
  void setupWishlistRoutes();
  void setupCollectionRoutes();
  void setupReleaseCalendarRoutes();
  void setupTagRoutes();
  void setupActionRoutes();
  void setupEnrichmentRoutes();
  void setupDealsRoutes();
  void setupSearchRoutes();
  void setupAnalyticsRoutes();
  void setupStaticRoutes();
  void setupWebSocketRoute();
  void setupSettingsRoutes();

  // HTML rendering
  std::string renderSPA();

  // Helper methods
  crow::json::wvalue wishlistItemToJson(const domain::WishlistItem &item);
  crow::json::wvalue collectionItemToJson(const domain::CollectionItem &item);
  crow::json::wvalue
  releaseCalendarItemToJson(const domain::ReleaseCalendarItem &item);
  crow::json::wvalue dealToJson(const domain::Deal &deal);
  std::string
  timePointToString(const std::chrono::system_clock::time_point &tp);

  crow::SimpleApp app_;
  std::shared_ptr<application::Scheduler> scheduler_;

  // WebSocket connections
  std::mutex ws_mutex_;
  std::set<crow::websocket::connection *> ws_connections_;

  // Background enrichment threads
  std::mutex threads_mutex_;
  std::vector<std::thread> background_threads_;

  std::unique_ptr<HtmlRenderer> renderer_;
};

} // namespace bluray::presentation
