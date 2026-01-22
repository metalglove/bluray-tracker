#include "tmdb_enrichment_service.hpp"
#include "../../infrastructure/repositories/wishlist_repository.hpp"
#include "../../infrastructure/repositories/collection_repository.hpp"
#include "../../infrastructure/logger.hpp"
#include <fmt/format.h>
#include <algorithm>
#include <regex>
#include <thread>
#include <cctype>

namespace bluray::application::enrichment {

// ============================================================================
// TmdbMatchingStrategy Implementation
// ============================================================================

std::optional<infrastructure::TmdbMovie> TmdbMatchingStrategy::findBestMatch(
    const std::vector<infrastructure::TmdbMovie>& results,
    std::string_view original_title,
    int year_hint
) {
    if (results.empty()) {
        return std::nullopt;
    }

    // Calculate confidence for each result
    std::vector<std::pair<double, const infrastructure::TmdbMovie*>> scored_results;
    scored_results.reserve(results.size());

    for (const auto& movie : results) {
        double confidence = calculateConfidence(movie, original_title, year_hint);
        scored_results.emplace_back(confidence, &movie);

        infrastructure::Logger::instance().debug(fmt::format(
            "TMDb match candidate: '{}' ({}) - confidence: {:.2f}",
            movie.title, movie.release_date.substr(0, 4), confidence
        ));
    }

    // Sort by confidence (highest first)
    std::sort(scored_results.begin(), scored_results.end(),
        [](const auto& a, const auto& b) {
            return a.first > b.first;
        });

    // Check if best match meets minimum threshold
    const auto& [best_confidence, best_movie] = scored_results[0];

    if (best_confidence < MIN_CONFIDENCE_THRESHOLD) {
        infrastructure::Logger::instance().warn(fmt::format(
            "Best TMDb match '{}' has low confidence: {:.2f} (threshold: {:.2f})",
            best_movie->title, best_confidence, MIN_CONFIDENCE_THRESHOLD
        ));
        return std::nullopt;
    }

    infrastructure::Logger::instance().info(fmt::format(
        "Selected TMDb match: '{}' ({}) with confidence {:.2f}",
        best_movie->title, best_movie->release_date.substr(0, 4), best_confidence
    ));

    // Create a copy and set confidence score
    infrastructure::TmdbMovie result = *best_movie;
    result.match_confidence = best_confidence;

    return result;
}

double TmdbMatchingStrategy::calculateTitleSimilarity(
    std::string_view title1,
    std::string_view title2
) {
    std::string norm1 = normalizeTitle(title1);
    std::string norm2 = normalizeTitle(title2);

    // Exact match after normalization
    if (norm1 == norm2) {
        return 1.0;
    }

    // Calculate Levenshtein distance
    int distance = levenshteinDistance(norm1, norm2);
    int max_length = std::max(norm1.length(), norm2.length());

    if (max_length == 0) {
        return 0.0;
    }

    // Convert distance to similarity (0.0 to 1.0)
    return 1.0 - (static_cast<double>(distance) / max_length);
}

std::string TmdbMatchingStrategy::normalizeTitle(std::string_view title) {
    std::string normalized;
    normalized.reserve(title.length());

    // Convert to lowercase and remove non-alphanumeric characters
    for (char c : title) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            normalized += std::tolower(static_cast<unsigned char>(c));
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            normalized += ' ';
        }
    }

    // Remove leading/trailing whitespace
    size_t start = normalized.find_first_not_of(' ');
    size_t end = normalized.find_last_not_of(' ');

    if (start == std::string::npos) {
        return "";
    }

    normalized = normalized.substr(start, end - start + 1);

    // Remove leading articles (the, a, an)
    const std::vector<std::string> articles = {"the ", "a ", "an "};
    for (const auto& article : articles) {
        if (normalized.find(article) == 0) {
            normalized = normalized.substr(article.length());
            break;
        }
    }

    // Remove multiple spaces
    normalized.erase(
        std::unique(normalized.begin(), normalized.end(),
            [](char a, char b) { return a == ' ' && b == ' '; }),
        normalized.end()
    );

    return normalized;
}

int TmdbMatchingStrategy::extractYearFromTitle(std::string_view title) {
    // Match (YYYY) or [YYYY] patterns
    std::regex year_regex(R"([\(\[](\d{4})[\)\]])");
    std::string title_str(title);
    std::smatch match;

    if (std::regex_search(title_str, match, year_regex)) {
        try {
            return std::stoi(match[1].str());
        } catch (...) {
            return 0;
        }
    }

    return 0;
}

int TmdbMatchingStrategy::levenshteinDistance(std::string_view s1, std::string_view s2) {
    const size_t m = s1.size();
    const size_t n = s2.size();

    // Create DP table
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

    // Initialize base cases
    for (size_t i = 0; i <= m; ++i) {
        dp[i][0] = static_cast<int>(i);
    }
    for (size_t j = 0; j <= n; ++j) {
        dp[0][j] = static_cast<int>(j);
    }

    // Fill DP table
    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (s1[i-1] == s2[j-1]) {
                dp[i][j] = dp[i-1][j-1];
            } else {
                dp[i][j] = 1 + std::min({
                    dp[i-1][j],    // deletion
                    dp[i][j-1],    // insertion
                    dp[i-1][j-1]   // substitution
                });
            }
        }
    }

    return dp[m][n];
}

double TmdbMatchingStrategy::calculateConfidence(
    const infrastructure::TmdbMovie& movie,
    std::string_view original_title,
    int year_hint
) {
    // Extract year from original title if not provided
    if (year_hint == 0) {
        year_hint = extractYearFromTitle(original_title);
    }

    // Calculate title similarity (70% weight)
    double title_similarity = calculateTitleSimilarity(original_title, movie.title);

    // Also check original title similarity
    if (!movie.original_title.empty() && movie.original_title != movie.title) {
        double original_similarity = calculateTitleSimilarity(original_title, movie.original_title);
        title_similarity = std::max(title_similarity, original_similarity);
    }

    // Calculate year proximity (20% weight)
    double year_proximity = 0.0;
    if (year_hint > 0 && !movie.release_date.empty()) {
        try {
            int movie_year = std::stoi(movie.release_date.substr(0, 4));
            int year_diff = std::abs(movie_year - year_hint);

            // Perfect match = 1.0, decreases with difference
            if (year_diff == 0) {
                year_proximity = 1.0;
            } else if (year_diff == 1) {
                year_proximity = 0.8;
            } else if (year_diff <= 3) {
                year_proximity = 0.5;
            } else {
                year_proximity = 0.2;
            }
        } catch (...) {
            year_proximity = 0.5; // Neutral if can't parse year
        }
    } else {
        year_proximity = 0.5; // Neutral if no year to compare
    }

    // Calculate popularity score (10% weight)
    // Normalize vote_average (0-10) to 0-1 scale
    double popularity = movie.vote_average / 10.0;

    // Weighted average
    double confidence = (title_similarity * 0.7) +
                       (year_proximity * 0.2) +
                       (popularity * 0.1);

    return confidence;
}

// ============================================================================
// TmdbEnrichmentService Implementation
// ============================================================================

TmdbEnrichmentService::TmdbEnrichmentService()
    : client_(std::make_unique<infrastructure::TmdbClient>()) {
}

TmdbEnrichmentService::TmdbEnrichmentService(
    std::unique_ptr<infrastructure::TmdbClient> client
)
    : client_(std::move(client)) {
}

EnrichmentResult TmdbEnrichmentService::enrichWishlistItem(
    domain::WishlistItem& item
) {
    infrastructure::Logger::instance().info(fmt::format(
        "Enriching wishlist item {} ('{}')",
        item.id, item.title
    ));

    // If item already has TMDb ID, fetch details directly
    if (item.tmdb_id > 0) {
        return enrichByImdbId(item.imdb_id.empty() ? "" : item.imdb_id);
    }

    // Otherwise, search by title
    EnrichmentResult result = enrichByTitle(item.title);

    if (result.success) {
        // Update item with enriched data
        item.tmdb_id = result.tmdb_id;
        item.imdb_id = result.imdb_id;
        item.tmdb_rating = result.tmdb_rating;
        item.trailer_key = result.trailer_key;

        infrastructure::Logger::instance().info(fmt::format(
            "Successfully enriched item {} with TMDb ID {} (confidence: {:.2f})",
            item.id, result.tmdb_id, result.confidence_score
        ));
    } else {
        infrastructure::Logger::instance().warn(fmt::format(
            "Failed to enrich item {}: {}",
            item.id, result.error_message
        ));
    }

    return result;
}

EnrichmentResult TmdbEnrichmentService::enrichCollectionItem(
    domain::CollectionItem& item
) {
    infrastructure::Logger::instance().info(fmt::format(
        "Enriching collection item {} ('{}')",
        item.id, item.title
    ));

    // If item already has TMDb ID, fetch details directly
    if (item.tmdb_id > 0) {
        return enrichByImdbId(item.imdb_id.empty() ? "" : item.imdb_id);
    }

    // Otherwise, search by title
    EnrichmentResult result = enrichByTitle(item.title);

    if (result.success) {
        // Update item with enriched data
        item.tmdb_id = result.tmdb_id;
        item.imdb_id = result.imdb_id;
        item.tmdb_rating = result.tmdb_rating;
        item.trailer_key = result.trailer_key;

        infrastructure::Logger::instance().info(fmt::format(
            "Successfully enriched collection item {} with TMDb ID {} (confidence: {:.2f})",
            item.id, result.tmdb_id, result.confidence_score
        ));
    } else {
        infrastructure::Logger::instance().warn(fmt::format(
            "Failed to enrich collection item {}: {}",
            item.id, result.error_message
        ));
    }

    return result;
}

BulkEnrichmentProgress TmdbEnrichmentService::enrichMultipleWishlistItems(
    const std::vector<int>& item_ids,
    std::function<void(const BulkEnrichmentProgress&)> progress_callback
) {
    std::lock_guard<std::mutex> lock(mutex_);

    bulk_progress_ = BulkEnrichmentProgress{};
    bulk_progress_.total = static_cast<int>(item_ids.size());
    bulk_progress_.is_active = true;

    infrastructure::Logger::instance().info(fmt::format(
        "Starting bulk enrichment of {} wishlist items",
        bulk_progress_.total
    ));

    infrastructure::SqliteWishlistRepository repository;

    for (int item_id : item_ids) {
        bulk_progress_.current_item_id = item_id;

        // Load item from repository
        auto item_opt = repository.findById(item_id);
        if (!item_opt) {
            infrastructure::Logger::instance().warn(fmt::format(
                "Wishlist item {} not found, skipping",
                item_id
            ));
            bulk_progress_.failed++;
            bulk_progress_.processed++;
            if (progress_callback) {
                progress_callback(bulk_progress_);
            }
            continue;
        }

        auto item = *item_opt;

        // Enrich item
        EnrichmentResult result = enrichWishlistItem(item);

        if (result.success) {
            // Save enriched item back to repository
            if (repository.update(item)) {
                bulk_progress_.successful++;
            } else {
                bulk_progress_.failed++;
                infrastructure::Logger::instance().error(fmt::format(
                    "Failed to save enriched wishlist item {}",
                    item_id
                ));
            }
        } else {
            bulk_progress_.failed++;
        }

        bulk_progress_.processed++;

        // Call progress callback
        if (progress_callback) {
            progress_callback(bulk_progress_);
        }

        // Rate limiting delay (except for last item)
        if (bulk_progress_.processed < bulk_progress_.total) {
            sleepForRateLimit();
        }
    }

    bulk_progress_.is_active = false;

    infrastructure::Logger::instance().info(fmt::format(
        "Bulk enrichment complete: {}/{} successful, {} failed",
        bulk_progress_.successful, bulk_progress_.total, bulk_progress_.failed
    ));

    return bulk_progress_;
}

BulkEnrichmentProgress TmdbEnrichmentService::enrichMultipleCollectionItems(
    const std::vector<int>& item_ids,
    std::function<void(const BulkEnrichmentProgress&)> progress_callback
) {
    std::lock_guard<std::mutex> lock(mutex_);

    bulk_progress_ = BulkEnrichmentProgress{};
    bulk_progress_.total = static_cast<int>(item_ids.size());
    bulk_progress_.is_active = true;

    infrastructure::Logger::instance().info(fmt::format(
        "Starting bulk enrichment of {} collection items",
        bulk_progress_.total
    ));

    infrastructure::SqliteCollectionRepository repository;

    for (int item_id : item_ids) {
        bulk_progress_.current_item_id = item_id;

        // Load item from repository
        auto item_opt = repository.findById(item_id);
        if (!item_opt) {
            infrastructure::Logger::instance().warn(fmt::format(
                "Collection item {} not found, skipping",
                item_id
            ));
            bulk_progress_.failed++;
            bulk_progress_.processed++;
            if (progress_callback) {
                progress_callback(bulk_progress_);
            }
            continue;
        }

        auto item = *item_opt;

        // Enrich item
        EnrichmentResult result = enrichCollectionItem(item);

        if (result.success) {
            // Save enriched item back to repository
            if (repository.update(item)) {
                bulk_progress_.successful++;
            } else {
                bulk_progress_.failed++;
                infrastructure::Logger::instance().error(fmt::format(
                    "Failed to save enriched collection item {}",
                    item_id
                ));
            }
        } else {
            bulk_progress_.failed++;
        }

        bulk_progress_.processed++;

        // Call progress callback
        if (progress_callback) {
            progress_callback(bulk_progress_);
        }

        // Rate limiting delay (except for last item)
        if (bulk_progress_.processed < bulk_progress_.total) {
            sleepForRateLimit();
        }
    }

    bulk_progress_.is_active = false;

    infrastructure::Logger::instance().info(fmt::format(
        "Bulk enrichment complete: {}/{} successful, {} failed",
        bulk_progress_.successful, bulk_progress_.total, bulk_progress_.failed
    ));

    return bulk_progress_;
}

bool TmdbEnrichmentService::isEnabled() const {
    return client_->hasApiKey();
}

BulkEnrichmentProgress TmdbEnrichmentService::getCurrentProgress() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bulk_progress_;
}

EnrichmentResult TmdbEnrichmentService::enrichByTitle(std::string_view title) {
    EnrichmentResult result;

    if (!client_->hasApiKey()) {
        result.error_message = "TMDb API key not configured";
        return result;
    }

    // Extract year from title if present
    int year_hint = TmdbMatchingStrategy::extractYearFromTitle(title);

    infrastructure::Logger::instance().debug(fmt::format(
        "Searching TMDb for '{}' (year hint: {})",
        title, year_hint > 0 ? std::to_string(year_hint) : "none"
    ));

    // Search TMDb
    auto search_result = client_->searchMovie(title, year_hint);

    if (!search_result || search_result->results.empty()) {
        result.error_message = "No TMDb results found for this title";
        return result;
    }

    // Find best match
    auto best_match = TmdbMatchingStrategy::findBestMatch(
        search_result->results,
        title,
        year_hint
    );

    if (!best_match) {
        result.error_message = fmt::format(
            "No confident match found (best confidence below {:.0f}%)",
            TmdbMatchingStrategy::MIN_CONFIDENCE_THRESHOLD * 100
        );
        return result;
    }

    // Get detailed movie information
    auto movie_details = client_->getMovieDetails(best_match->id);

    if (!movie_details) {
        result.error_message = "Failed to fetch movie details from TMDb";
        return result;
    }

    // Get trailer
    auto trailer_key = getBestTrailer(movie_details->id);

    // Populate result
    result.success = true;
    result.tmdb_id = movie_details->id;
    result.imdb_id = movie_details->imdb_id;
    result.tmdb_rating = movie_details->vote_average;
    result.trailer_key = trailer_key.value_or("");
    result.confidence_score = best_match->match_confidence;
    result.overview = movie_details->overview;
    result.runtime = movie_details->runtime;
    result.release_date = movie_details->release_date;

    return result;
}

EnrichmentResult TmdbEnrichmentService::enrichByImdbId(std::string_view imdb_id) {
    EnrichmentResult result;

    if (!client_->hasApiKey()) {
        result.error_message = "TMDb API key not configured";
        return result;
    }

    if (imdb_id.empty()) {
        result.error_message = "IMDb ID is empty";
        return result;
    }

    infrastructure::Logger::instance().debug(fmt::format(
        "Searching TMDb by IMDb ID '{}'",
        imdb_id
    ));

    // Find by IMDb ID
    auto movie = client_->findByImdbId(imdb_id);

    if (!movie) {
        result.error_message = "No TMDb result found for this IMDb ID";
        return result;
    }

    // Get detailed information (for runtime, etc.)
    auto movie_details = client_->getMovieDetails(movie->id);

    if (!movie_details) {
        result.error_message = "Failed to fetch movie details from TMDb";
        return result;
    }

    // Get trailer
    auto trailer_key = getBestTrailer(movie_details->id);

    // Populate result
    result.success = true;
    result.tmdb_id = movie_details->id;
    result.imdb_id = std::string(imdb_id);
    result.tmdb_rating = movie_details->vote_average;
    result.trailer_key = trailer_key.value_or("");
    result.confidence_score = 1.0; // IMDb ID lookup is definitive
    result.overview = movie_details->overview;
    result.runtime = movie_details->runtime;
    result.release_date = movie_details->release_date;

    return result;
}

std::optional<std::string> TmdbEnrichmentService::getBestTrailer(int tmdb_id) {
    auto videos = client_->getMovieVideos(tmdb_id);

    if (videos.empty()) {
        return std::nullopt;
    }

    // Priority order: Official Trailer > Trailer > Official Teaser > Teaser
    std::vector<std::string> priority_types = {
        "Trailer|true",  // Official trailer
        "Trailer|false", // Non-official trailer
        "Teaser|true",   // Official teaser
        "Teaser|false"   // Non-official teaser
    };

    for (const auto& priority_key : priority_types) {
        auto delimiter_pos = priority_key.find('|');
        std::string type = priority_key.substr(0, delimiter_pos);
        bool is_official = (priority_key.substr(delimiter_pos + 1) == "true");

        for (const auto& video : videos) {
            // Only YouTube videos
            if (video.site != "YouTube") {
                continue;
            }

            if (video.type == type && video.official == is_official) {
                infrastructure::Logger::instance().debug(fmt::format(
                    "Selected {} trailer: '{}' (key: {})",
                    video.official ? "official" : "non-official",
                    video.name, video.key
                ));
                return video.key;
            }
        }
    }

    // Fallback: return first YouTube video
    for (const auto& video : videos) {
        if (video.site == "YouTube" && !video.key.empty()) {
            infrastructure::Logger::instance().debug(fmt::format(
                "Selected fallback video: '{}' (key: {})",
                video.name, video.key
            ));
            return video.key;
        }
    }

    return std::nullopt;
}

void TmdbEnrichmentService::sleepForRateLimit() {
    // Conservative delay: 250ms per request
    // This allows 240 requests per minute, well under TMDb's 40 req/10 sec limit
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
}

} // namespace bluray::application::enrichment
