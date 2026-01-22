#pragma once

#include "network_client.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>
#include <chrono>
#include <mutex>

namespace bluray::infrastructure {

/**
 * TMDb API v3 response structures
 */
struct TmdbMovie {
    int id{0};
    std::string title;
    std::string original_title;
    std::string imdb_id;
    double vote_average{0.0};
    std::string release_date;
    std::string overview;
    std::string poster_path;
    std::string backdrop_path;
    std::vector<int> genre_ids;
    int runtime{0};

    // Confidence score for matching (0.0 - 1.0)
    double match_confidence{0.0};
};

struct TmdbVideo {
    std::string key;        // YouTube video ID
    std::string name;
    std::string type;       // "Trailer", "Teaser", etc.
    std::string site;       // "YouTube"
    bool official{false};
};

struct TmdbSearchResult {
    std::vector<TmdbMovie> results;
    int total_results{0};
    int page{1};
    int total_pages{1};
};

/**
 * Rate limiting state
 */
struct RateLimitState {
    int requests_made{0};
    std::chrono::steady_clock::time_point window_start;
    static constexpr int MAX_REQUESTS_PER_10_SEC = 40;  // TMDb free tier limit
};

/**
 * TMDb API client for fetching movie metadata
 *
 * Provides thread-safe access to TMDb API v3 with built-in rate limiting
 * to respect the free tier limit of 40 requests per 10 seconds.
 *
 * Usage:
 *   TmdbClient client("your_api_key");
 *   auto results = client.searchMovie("The Matrix", 1999);
 *   if (results) {
 *     for (const auto& movie : results->results) {
 *       // Process movie data
 *     }
 *   }
 */
class TmdbClient {
public:
    /**
     * Default constructor - loads API key from ConfigManager
     */
    TmdbClient();

    /**
     * Constructor with explicit API key
     * @param api_key TMDb API v3 key
     */
    explicit TmdbClient(std::string_view api_key);

    /**
     * Search movies by title
     * @param query Movie title to search for
     * @param year Optional year filter (0 = no filter)
     * @param page Page number for pagination (default: 1)
     * @return Search results or nullopt on error
     */
    std::optional<TmdbSearchResult> searchMovie(
        std::string_view query,
        int year = 0,
        int page = 1
    );

    /**
     * Get detailed movie information by TMDb ID
     * Includes external IDs (IMDb) via append_to_response
     * @param movie_id TMDb movie ID
     * @return Movie details or nullopt on error
     */
    std::optional<TmdbMovie> getMovieDetails(int movie_id);

    /**
     * Get movie videos (trailers, teasers, etc.)
     * @param movie_id TMDb movie ID
     * @return Vector of video entries (empty on error)
     */
    std::vector<TmdbVideo> getMovieVideos(int movie_id);

    /**
     * Find movie by IMDb ID
     * @param imdb_id IMDb ID (e.g., "tt0133093")
     * @return Movie details or nullopt on error
     */
    std::optional<TmdbMovie> findByImdbId(std::string_view imdb_id);

    /**
     * Set or update API key
     * @param api_key TMDb API v3 key
     */
    void setApiKey(std::string_view api_key);

    /**
     * Check if API key is configured
     * @return true if API key is set and non-empty
     */
    bool hasApiKey() const;

    /**
     * Get current rate limit status
     * @return RateLimitState with requests made and window start time
     */
    RateLimitState getRateLimitState() const;

private:
    /**
     * Build full TMDb API URL with query parameters
     * @param endpoint API endpoint (e.g., "/search/movie")
     * @param params Query parameters (excluding api_key)
     * @return Full URL string
     */
    std::string buildUrl(
        std::string_view endpoint,
        const std::vector<std::pair<std::string, std::string>>& params = {}
    );

    /**
     * Make HTTP GET request to TMDb API
     * @param url Full URL to request
     * @return Parsed JSON response or nullopt on error
     */
    std::optional<nlohmann::json> makeRequest(std::string_view url);

    /**
     * Check if we can make another request without exceeding rate limit
     * Resets window if 10 seconds have elapsed
     * @return true if request is allowed, false if rate limit exceeded
     */
    bool checkRateLimit();

    /**
     * Increment rate limit counter after successful request
     */
    void incrementRateLimit();

    /**
     * Parse TMDb movie JSON object into TmdbMovie struct
     * @param json JSON object from TMDb API
     * @return TmdbMovie with populated fields
     */
    TmdbMovie parseMovie(const nlohmann::json& json);

    /**
     * Parse TMDb video JSON object into TmdbVideo struct
     * @param json JSON object from TMDb API
     * @return TmdbVideo with populated fields
     */
    TmdbVideo parseVideo(const nlohmann::json& json);

    NetworkClient client_;
    std::string api_key_;
    RateLimitState rate_limit_state_;
    mutable std::mutex mutex_;

    static constexpr const char* BASE_URL = "https://api.themoviedb.org/3";
};

} // namespace bluray::infrastructure
