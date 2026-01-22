#include "tmdb_client.hpp"
#include "config_manager.hpp"
#include "logger.hpp"
#include <fmt/format.h>
#include <algorithm>
#include <thread>

namespace bluray::infrastructure {

TmdbClient::TmdbClient() {
  // Load API key from config
  const auto& config = ConfigManager::instance();
  api_key_ = config.get("tmdb_api_key", "");

  // Initialize rate limit window
  rate_limit_state_.window_start = std::chrono::steady_clock::now();
}

TmdbClient::TmdbClient(std::string_view api_key)
    : api_key_(api_key) {
  rate_limit_state_.window_start = std::chrono::steady_clock::now();
}

std::optional<TmdbSearchResult> TmdbClient::searchMovie(
    std::string_view query,
    int year,
    int page
) {
  if (!hasApiKey()) {
    Logger::instance().error("TMDb API key not configured");
    return std::nullopt;
  }

  std::vector<std::pair<std::string, std::string>> params = {
      {"query", std::string(query)},
      {"page", std::to_string(page)}
  };

  if (year > 0) {
    params.push_back({"year", std::to_string(year)});
  }

  const std::string url = buildUrl("/search/movie", params);
  auto json_opt = makeRequest(url);

  if (!json_opt) {
    return std::nullopt;
  }

  try {
    const auto& json = *json_opt;
    TmdbSearchResult result;
    result.page = json.value("page", 1);
    result.total_results = json.value("total_results", 0);
    result.total_pages = json.value("total_pages", 1);

    if (json.contains("results") && json["results"].is_array()) {
      for (const auto& movie_json : json["results"]) {
        result.results.push_back(parseMovie(movie_json));
      }
    }

    Logger::instance().debug(fmt::format(
        "TMDb search for '{}' returned {} results",
        query, result.total_results
    ));

    return result;
  } catch (const std::exception& e) {
    Logger::instance().error(fmt::format(
        "Failed to parse TMDb search results: {}", e.what()
    ));
    return std::nullopt;
  }
}

std::optional<TmdbMovie> TmdbClient::getMovieDetails(int movie_id) {
  if (!hasApiKey()) {
    Logger::instance().error("TMDb API key not configured");
    return std::nullopt;
  }

  std::vector<std::pair<std::string, std::string>> params = {
      {"append_to_response", "external_ids"}
  };

  const std::string url = buildUrl(
      fmt::format("/movie/{}", movie_id),
      params
  );

  auto json_opt = makeRequest(url);

  if (!json_opt) {
    return std::nullopt;
  }

  try {
    const auto& json = *json_opt;
    TmdbMovie movie = parseMovie(json);

    // Extract IMDb ID from external_ids
    if (json.contains("external_ids") && json["external_ids"].is_object()) {
      const auto& external_ids = json["external_ids"];
      if (external_ids.contains("imdb_id") && !external_ids["imdb_id"].is_null()) {
        movie.imdb_id = external_ids["imdb_id"].get<std::string>();
      }
    }

    // Extract runtime
    if (json.contains("runtime") && !json["runtime"].is_null()) {
      movie.runtime = json["runtime"].get<int>();
    }

    Logger::instance().debug(fmt::format(
        "TMDb fetched details for movie ID {}: '{}'",
        movie_id, movie.title
    ));

    return movie;
  } catch (const std::exception& e) {
    Logger::instance().error(fmt::format(
        "Failed to parse TMDb movie details: {}", e.what()
    ));
    return std::nullopt;
  }
}

std::vector<TmdbVideo> TmdbClient::getMovieVideos(int movie_id) {
  if (!hasApiKey()) {
    Logger::instance().error("TMDb API key not configured");
    return {};
  }

  const std::string url = buildUrl(fmt::format("/movie/{}/videos", movie_id));
  auto json_opt = makeRequest(url);

  if (!json_opt) {
    return {};
  }

  try {
    const auto& json = *json_opt;
    std::vector<TmdbVideo> videos;

    if (json.contains("results") && json["results"].is_array()) {
      for (const auto& video_json : json["results"]) {
        videos.push_back(parseVideo(video_json));
      }
    }

    Logger::instance().debug(fmt::format(
        "TMDb fetched {} videos for movie ID {}",
        videos.size(), movie_id
    ));

    return videos;
  } catch (const std::exception& e) {
    Logger::instance().error(fmt::format(
        "Failed to parse TMDb videos: {}", e.what()
    ));
    return {};
  }
}

std::optional<TmdbMovie> TmdbClient::findByImdbId(std::string_view imdb_id) {
  if (!hasApiKey()) {
    Logger::instance().error("TMDb API key not configured");
    return std::nullopt;
  }

  std::vector<std::pair<std::string, std::string>> params = {
      {"external_source", "imdb_id"}
  };

  const std::string url = buildUrl(
      fmt::format("/find/{}", imdb_id),
      params
  );

  auto json_opt = makeRequest(url);

  if (!json_opt) {
    return std::nullopt;
  }

  try {
    const auto& json = *json_opt;

    if (json.contains("movie_results") && json["movie_results"].is_array()) {
      const auto& movie_results = json["movie_results"];
      if (!movie_results.empty()) {
        TmdbMovie movie = parseMovie(movie_results[0]);
        movie.imdb_id = std::string(imdb_id);

        Logger::instance().debug(fmt::format(
            "TMDb found movie for IMDb ID {}: '{}'",
            imdb_id, movie.title
        ));

        return movie;
      }
    }

    Logger::instance().warning(fmt::format(
        "TMDb found no movie for IMDb ID {}",
        imdb_id
    ));

    return std::nullopt;
  } catch (const std::exception& e) {
    Logger::instance().error(fmt::format(
        "Failed to parse TMDb find results: {}", e.what()
    ));
    return std::nullopt;
  }
}

void TmdbClient::setApiKey(std::string_view api_key) {
  std::lock_guard<std::mutex> lock(mutex_);
  api_key_ = api_key;
}

bool TmdbClient::hasApiKey() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return !api_key_.empty();
}

RateLimitState TmdbClient::getRateLimitState() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return rate_limit_state_;
}

std::string TmdbClient::buildUrl(
    std::string_view endpoint,
    const std::vector<std::pair<std::string, std::string>>& params
) {
  std::string url = fmt::format("{}{}", BASE_URL, endpoint);

  // URL encode helper using RFC 3986 unreserved characters
  auto url_encode = [](std::string_view str) -> std::string {
    std::string encoded;
    encoded.reserve(str.length() * 3); // Reserve worst case

    for (char c : str) {
      unsigned char uc = static_cast<unsigned char>(c);
      // RFC 3986: unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
      if (std::isalnum(uc) || c == '-' || c == '_' || c == '.' || c == '~') {
        encoded += c;
      } else {
        // Percent-encode all other characters
        encoded += fmt::format("%{:02X}", uc);
      }
    }

    return encoded;
  };

  // Build URL with query parameters (no API key in URL)
  bool first_param = true;
  for (const auto& [key, value] : params) {
    url += first_param ? "?" : "&";
    url += fmt::format("{}={}", key, url_encode(value));
    first_param = false;
  }

  return url;
}

std::optional<nlohmann::json> TmdbClient::makeRequest(std::string_view url) {
  // Check rate limit before making request
  if (!checkRateLimit()) {
    Logger::instance().warning("TMDb rate limit exceeded, waiting...");

    int wait_time = 0;
    {
      // Calculate time to wait until window reset
      std::lock_guard<std::mutex> lock(mutex_);
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          now - rate_limit_state_.window_start
      ).count();

      wait_time = static_cast<int>(std::max(0L, 10L - elapsed));
    }

    if (wait_time > 0) {
      std::this_thread::sleep_for(std::chrono::seconds(wait_time));
    }

    {
      // Reset window
      std::lock_guard<std::mutex> lock(mutex_);
      rate_limit_state_.window_start = std::chrono::steady_clock::now();
      rate_limit_state_.requests_made = 0;
    }
  }

  // Make HTTP request with Authorization header (Bearer token authentication)
  std::vector<std::string> headers = {
    fmt::format("Authorization: Bearer {}", api_key_)
  };
  auto response = client_.get(url, headers);

  if (!response.success) {
    Logger::instance().error(fmt::format(
        "TMDb API request failed: HTTP {}",
        response.status_code
    ));

    if (response.status_code == 401) {
      Logger::instance().error("Invalid TMDb API key");
    } else if (response.status_code == 429) {
      Logger::instance().error("TMDb rate limit exceeded (429)");
    }

    return std::nullopt;
  }

  // Increment rate limit counter
  incrementRateLimit();

  // Parse JSON response
  try {
    return nlohmann::json::parse(response.body);
  } catch (const std::exception& e) {
    Logger::instance().error(fmt::format(
        "Failed to parse TMDb JSON response: {}", e.what()
    ));
    return std::nullopt;
  }
}

bool TmdbClient::checkRateLimit() {
  std::lock_guard<std::mutex> lock(mutex_);

  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
      now - rate_limit_state_.window_start
  ).count();

  // Reset window if 10 seconds have elapsed
  if (elapsed >= 10) {
    rate_limit_state_.window_start = now;
    rate_limit_state_.requests_made = 0;
  }

  // Check if we can make another request
  return rate_limit_state_.requests_made < RateLimitState::MAX_REQUESTS_PER_10_SEC;
}

void TmdbClient::incrementRateLimit() {
  std::lock_guard<std::mutex> lock(mutex_);
  rate_limit_state_.requests_made++;
}

TmdbMovie TmdbClient::parseMovie(const nlohmann::json& json) {
  TmdbMovie movie;

  movie.id = json.value("id", 0);
  movie.title = json.value("title", "");
  movie.original_title = json.value("original_title", "");
  movie.vote_average = json.value("vote_average", 0.0);
  movie.release_date = json.value("release_date", "");
  movie.overview = json.value("overview", "");
  movie.poster_path = json.value("poster_path", "");
  movie.backdrop_path = json.value("backdrop_path", "");

  // Parse genre IDs
  if (json.contains("genre_ids") && json["genre_ids"].is_array()) {
    for (const auto& genre_id : json["genre_ids"]) {
      if (genre_id.is_number()) {
        movie.genre_ids.push_back(genre_id.get<int>());
      }
    }
  }

  return movie;
}

TmdbVideo TmdbClient::parseVideo(const nlohmann::json& json) {
  TmdbVideo video;

  video.key = json.value("key", "");
  video.name = json.value("name", "");
  video.type = json.value("type", "");
  video.site = json.value("site", "");
  video.official = json.value("official", false);

  return video;
}

} // namespace bluray::infrastructure
