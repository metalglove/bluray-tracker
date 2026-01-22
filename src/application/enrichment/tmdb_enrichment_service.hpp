#pragma once

#include "../../domain/models.hpp"
#include "../../infrastructure/tmdb_client.hpp"
#include <memory>
#include <optional>
#include <functional>
#include <mutex>

namespace bluray::application::enrichment {

/**
 * Result of an enrichment operation
 */
struct EnrichmentResult {
    bool success{false};
    int tmdb_id{0};
    std::string imdb_id;
    double tmdb_rating{0.0};
    std::string trailer_key;
    double confidence_score{0.0};
    std::string error_message;

    // Additional metadata (not stored yet, available for future use)
    std::string overview;
    std::vector<std::string> genres;
    int runtime{0};
    std::string release_date;
};

/**
 * Progress tracking for bulk enrichment operations
 */
struct BulkEnrichmentProgress {
    int total{0};
    int processed{0};
    int successful{0};
    int failed{0};
    bool is_active{false};
    int current_item_id{0};
};

/**
 * Strategy class for matching TMDb search results to original titles
 */
class TmdbMatchingStrategy {
public:
    /**
     * Find best match from TMDb search results
     * Uses title similarity, year proximity, and popularity
     *
     * @param results TMDb search results to match against
     * @param original_title Original movie title from user
     * @param year_hint Optional year hint (0 = no hint)
     * @return Best matching movie or nullopt if no good match found
     */
    static std::optional<infrastructure::TmdbMovie> findBestMatch(
        const std::vector<infrastructure::TmdbMovie>& results,
        std::string_view original_title,
        int year_hint = 0
    );

    /**
     * Extract year from title if present (e.g., "Movie (2023)" -> 2023)
     * @param title Title possibly containing year
     * @return Extracted year or 0 if not found
     */
    static int extractYearFromTitle(std::string_view title);

    // Minimum confidence threshold for accepting a match
    static constexpr double MIN_CONFIDENCE_THRESHOLD = 0.7;

private:
    /**
     * Calculate similarity between two titles using Levenshtein distance
     * @return Similarity score 0.0 to 1.0 (1.0 = identical)
     */
    static double calculateTitleSimilarity(
        std::string_view title1,
        std::string_view title2
    );

    /**
     * Normalize title for matching (lowercase, remove articles, special chars)
     * @param title Original title
     * @return Normalized title
     */
    static std::string normalizeTitle(std::string_view title);

    /**
     * Calculate Levenshtein distance between two strings
     * @return Edit distance (lower is more similar)
     */
    static int levenshteinDistance(std::string_view s1, std::string_view s2);

    /**
     * Calculate confidence score for a match
     * Formula: (Title Similarity × 0.7) + (Year Proximity × 0.2) + (Popularity × 0.1)
     *
     * @param movie TMDb movie candidate
     * @param original_title Original title to match
     * @param year_hint Year hint (0 = no hint)
     * @return Confidence score 0.0 to 1.0
     */
    static double calculateConfidence(
        const infrastructure::TmdbMovie& movie,
        std::string_view original_title,
        int year_hint
    );
};

/**
 * Service for enriching wishlist and collection items with TMDb metadata
 *
 * Orchestrates the enrichment workflow:
 * 1. Search TMDb by title (with year extraction)
 * 2. Find best match using smart matching algorithm
 * 3. Fetch detailed metadata including IMDb ID
 * 4. Get trailer videos
 * 5. Update item with enriched data
 *
 * Supports both single-item and bulk enrichment operations.
 * Bulk operations automatically handle rate limiting with progress callbacks.
 */
class TmdbEnrichmentService {
public:
    /**
     * Constructor - initializes TMDb client
     */
    TmdbEnrichmentService();

    /**
     * Explicit constructor with TMDb client
     * @param client Custom TmdbClient instance
     */
    explicit TmdbEnrichmentService(std::unique_ptr<infrastructure::TmdbClient> client);

    /**
     * Enrich wishlist item with TMDb metadata
     *
     * Modifies the item in-place with:
     * - tmdb_id
     * - imdb_id
     * - tmdb_rating
     * - trailer_key
     *
     * @param item Wishlist item to enrich (modified in-place)
     * @return EnrichmentResult with success status and confidence score
     */
    EnrichmentResult enrichWishlistItem(domain::WishlistItem& item);

    /**
     * Enrich collection item with TMDb metadata
     *
     * @param item Collection item to enrich (modified in-place)
     * @return EnrichmentResult with success status and confidence score
     */
    EnrichmentResult enrichCollectionItem(domain::CollectionItem& item);

    /**
     * Bulk enrich multiple wishlist items
     *
     * Processes items sequentially with automatic rate limiting delays (250ms).
     * Progress callback is invoked after each item is processed.
     *
     * @param item_ids Vector of wishlist item IDs to enrich
     * @param progress_callback Optional callback for progress updates
     * @return Final bulk enrichment progress
     */
    BulkEnrichmentProgress enrichMultipleWishlistItems(
        const std::vector<int>& item_ids,
        std::function<void(const BulkEnrichmentProgress&)> progress_callback = nullptr
    );

    /**
     * Bulk enrich multiple collection items
     *
     * @param item_ids Vector of collection item IDs to enrich
     * @param progress_callback Optional callback for progress updates
     * @return Final bulk enrichment progress
     */
    BulkEnrichmentProgress enrichMultipleCollectionItems(
        const std::vector<int>& item_ids,
        std::function<void(const BulkEnrichmentProgress&)> progress_callback = nullptr
    );

    /**
     * Check if TMDb enrichment is enabled
     * @return true if API key is configured
     */
    bool isEnabled() const;

    /**
     * Get current bulk enrichment progress
     * @return BulkEnrichmentProgress struct
     */
    BulkEnrichmentProgress getCurrentProgress() const;

private:
    /**
     * Core enrichment logic by title search
     * @param title Movie title to search
     * @return EnrichmentResult with metadata
     */
    EnrichmentResult enrichByTitle(std::string_view title);

    /**
     * Core enrichment logic by IMDb ID (reverse lookup)
     * @param imdb_id IMDb ID (e.g., "tt0133093")
     * @return EnrichmentResult with metadata
     */
    EnrichmentResult enrichByImdbId(std::string_view imdb_id);

    /**
     * Get best trailer from movie videos
     * Prioritizes: Official trailers > Trailers > Teasers
     *
     * @param tmdb_id TMDb movie ID
     * @return YouTube video key or empty optional
     */
    std::optional<std::string> getBestTrailer(int tmdb_id);

    /**
     * Sleep for rate limiting between bulk operations
     * Conservative delay: 250ms per request (240 requests per minute)
     */
    void sleepForRateLimit();

    std::unique_ptr<infrastructure::TmdbClient> client_;
    BulkEnrichmentProgress bulk_progress_;
    mutable std::mutex progress_mutex_;  // Separate mutex for progress queries
    mutable std::mutex operation_mutex_; // Mutex for operation control
};

} // namespace bluray::application::enrichment
