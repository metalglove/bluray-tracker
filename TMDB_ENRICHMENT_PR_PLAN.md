# TMDb API Enrichment - Pull Request Plan

## Overview

**Feature**: Automatic TMDb API integration for metadata enrichment
**Status**: Planning Complete - Ready for Implementation
**Estimated Effort**: 3-4 working days (22-28 hours)
**Priority**: High (Phase 2, top of roadmap)

### Problem Statement

Currently, users must manually enter TMDb IDs, IMDb IDs, ratings, and trailer keys for each movie. This is tedious and error-prone. While the database fields and UI exist, there's no automatic way to populate this metadata.

### Proposed Solution

Integrate with TMDb API v3 to automatically fetch and populate movie metadata when adding items to wishlist or collection. Include smart title matching, confidence scoring, rate limiting, and comprehensive error handling.

---

## Features Delivered

### Core Functionality
- ✅ Automatic metadata enrichment on item add (optional, configurable)
- ✅ Manual enrichment button for individual items
- ✅ Bulk enrichment for existing items without metadata
- ✅ Smart title matching with confidence scoring
- ✅ YouTube trailer key extraction
- ✅ TMDb/IMDb ID population
- ✅ Rating display with existing UI

### User Experience
- ✅ Settings page configuration for TMDb API key
- ✅ Auto-enrich toggle (on/off)
- ✅ Enrich-on-add toggle (default: enabled)
- ✅ Progress indicators for bulk operations
- ✅ WebSocket real-time updates
- ✅ Confidence score display
- ✅ Manual override capability

### Technical Features
- ✅ Rate limiting (40 requests per 10 seconds)
- ✅ Retry logic for network failures
- ✅ Thread-safe operations
- ✅ Comprehensive error handling
- ✅ Enrichment logging for debugging
- ✅ Graceful degradation (app works without TMDb)

---

## Architecture

### New Components

#### Infrastructure Layer
```
src/infrastructure/
├── tmdb_client.hpp         # TMDb API v3 HTTP client
└── tmdb_client.cpp         # Implementation with rate limiting
```

**Responsibilities:**
- HTTP requests to TMDb API
- JSON response parsing
- Rate limit enforcement
- Error handling and retries

#### Application Layer
```
src/application/enrichment/
├── tmdb_enrichment_service.hpp    # Enrichment orchestration
└── tmdb_enrichment_service.cpp    # Matching algorithm
```

**Responsibilities:**
- Title matching and confidence scoring
- Enrichment workflow (search → match → update)
- Bulk operation management
- Progress tracking

#### Presentation Layer Updates
```
src/presentation/
├── web_frontend.cpp        # New enrichment API endpoints
└── html_renderer.cpp       # UI updates for enrichment buttons
```

**Responsibilities:**
- REST API endpoints for enrichment
- WebSocket broadcasts for progress
- Settings UI for TMDb configuration

---

## Implementation Plan

### Phase 1: Infrastructure (Days 1-2)

**Goal**: Core TMDb client working with rate limiting

#### 1.1 Create TmdbClient Class
- [ ] Create `src/infrastructure/tmdb_client.hpp`
- [ ] Define data structures: `TmdbMovie`, `TmdbVideo`, `TmdbSearchResult`
- [ ] Define interface: `searchMovie()`, `getMovieDetails()`, `getMovieVideos()`
- [ ] Add rate limiting struct: `RateLimitState`

**Key Design Decisions:**
- Use existing `NetworkClient` pattern for HTTP
- Return `std::optional` for operations that may fail
- Thread-safe with `std::mutex`
- Sliding window rate limiting (40 req / 10 sec)

#### 1.2 Implement TmdbClient
- [ ] Create `src/infrastructure/tmdb_client.cpp`
- [ ] Implement `searchMovie()` - `/search/movie` endpoint
- [ ] Implement `getMovieDetails()` - `/movie/{id}` endpoint with `append_to_response=external_ids`
- [ ] Implement `getMovieVideos()` - `/movie/{id}/videos` endpoint
- [ ] Implement rate limiting logic
- [ ] Add comprehensive error handling

**Testing Checklist:**
- [ ] Search for "The Matrix" → Returns tmdb_id: 603
- [ ] Get details for tmdb_id 603 → Returns IMDb ID, rating, etc.
- [ ] Get videos for tmdb_id 603 → Returns trailer key
- [ ] Exceed rate limit → Wait and retry
- [ ] Invalid API key → Clear error message

#### 1.3 Database Configuration
- [ ] Add config keys to initial schema:
  - `tmdb_api_key` (empty by default)
  - `tmdb_auto_enrich` (default: 0)
  - `tmdb_enrich_on_add` (default: 1)
  - `tmdb_cache_enabled` (default: 1)

**SQL Migration:**
```sql
INSERT OR IGNORE INTO config (key, value) VALUES
    ('tmdb_api_key', ''),
    ('tmdb_auto_enrich', '0'),
    ('tmdb_enrich_on_add', '1');
```

---

### Phase 2: Enrichment Service (Day 3)

**Goal**: Smart matching and enrichment logic working

#### 2.1 Create TmdbEnrichmentService
- [ ] Create `src/application/enrichment/tmdb_enrichment_service.hpp`
- [ ] Define `EnrichmentResult` struct with confidence score
- [ ] Define interface: `enrichWishlistItem()`, `enrichCollectionItem()`, `enrichMultiple()`
- [ ] Define `BulkEnrichmentProgress` struct for tracking

#### 2.2 Implement Matching Strategy
- [ ] Create `TmdbMatchingStrategy` class
- [ ] Implement `normalizeTitle()` - Remove special chars, lowercase
- [ ] Implement `calculateTitleSimilarity()` - Levenshtein distance or similar
- [ ] Implement `extractYearFromTitle()` - Regex for (YYYY) or [YYYY]
- [ ] Implement `findBestMatch()` - Scoring algorithm

**Matching Algorithm:**
```
Confidence Score = (Title Similarity × 0.7) + (Year Proximity × 0.2) + (Popularity × 0.1)

Accept if Confidence > 0.7
```

**Edge Cases:**
- Title contains year → Extract and use for filtering
- Multiple exact matches → Pick most popular (highest vote_count)
- No good matches → Return success=false with message

#### 2.3 Implement Enrichment Logic
- [ ] Implement `enrichWishlistItem()`:
  1. Check if TMDb API key configured
  2. Search by title (extract year if present)
  3. Find best match with confidence > 0.7
  4. Get full details + videos
  5. Update item fields (tmdb_id, imdb_id, rating, trailer_key)
  6. Return EnrichmentResult

- [ ] Implement `enrichCollectionItem()` (same logic)

- [ ] Implement `enrichMultipleWishlistItems()`:
  1. Load items from repository
  2. Loop with rate limiting (250ms delay between requests)
  3. Call enrichWishlistItem() for each
  4. Track progress and broadcast via callback
  5. Return final statistics

**Testing Checklist:**
- [ ] Enrich "The Matrix (1999)" → 100% confidence, correct metadata
- [ ] Enrich "Inception" → High confidence, correct metadata
- [ ] Enrich "asdfghjkl" → No match, success=false
- [ ] Bulk enrich 10 items → All processed, ~3 seconds total
- [ ] Enrich without API key → Error: "API key not configured"

---

### Phase 3: API Integration (Days 4-5)

**Goal**: REST endpoints and UI integration

#### 3.1 Add API Endpoints
- [ ] Update `src/presentation/web_frontend.cpp`
- [ ] Add `POST /api/wishlist/{id}/enrich` - Single item enrichment
- [ ] Add `POST /api/collection/{id}/enrich` - Single collection item
- [ ] Add `POST /api/enrich/bulk` - Bulk enrichment (async)
- [ ] Add `GET /api/enrich/progress` - Progress tracking
- [ ] Add `POST /api/enrich/auto` - Auto-enrich all unenriched items

**Endpoint Implementations:**

**Single Item Enrichment:**
```cpp
CROW_ROUTE(app_, "/api/wishlist/<int>/enrich")
    .methods("POST"_method)
    ([this](int id) {
        auto repo = SqliteWishlistRepository();
        auto item_opt = repo.findById(id);

        if (!item_opt) {
            return crow::response(404, "Item not found");
        }

        auto item = *item_opt;
        TmdbEnrichmentService service;
        auto result = service.enrichWishlistItem(item);

        if (result.success) {
            repo.update(item);  // Save enriched data

            // Broadcast to WebSocket clients
            broadcastWishlistUpdate(item);
        }

        nlohmann::json response = {
            {"success", result.success},
            {"tmdb_id", result.tmdb_id},
            {"imdb_id", result.imdb_id},
            {"tmdb_rating", result.tmdb_rating},
            {"trailer_key", result.trailer_key},
            {"confidence", result.confidence_score},
            {"error", result.error_message}
        };

        return crow::response(200, response.dump());
    });
```

**Bulk Enrichment:**
```cpp
CROW_ROUTE(app_, "/api/enrich/bulk")
    .methods("POST"_method)
    ([this](const crow::request& req) {
        auto json = nlohmann::json::parse(req.body);
        auto item_ids = json["item_ids"].get<std::vector<int>>();

        // Launch async task
        std::thread([this, item_ids]() {
            TmdbEnrichmentService service;
            service.enrichMultipleWishlistItems(item_ids, [this](const auto& progress) {
                // Broadcast progress via WebSocket
                nlohmann::json ws_msg = {
                    {"type", "enrichment_progress"},
                    {"processed", progress.processed},
                    {"total", progress.total},
                    {"successful", progress.successful},
                    {"failed", progress.failed}
                };
                broadcastMessage(ws_msg.dump());
            });
        }).detach();

        nlohmann::json response = {
            {"started", true},
            {"total", item_ids.size()}
        };
        return crow::response(200, response.dump());
    });
```

#### 3.2 Update Settings Endpoints
- [ ] Update `GET /api/settings` - Include TMDb config keys
- [ ] Update `PUT /api/settings` - Accept TMDb config updates
- [ ] **Important**: Never return `tmdb_api_key` in GET (security)

**Settings GET Response:**
```json
{
  "scrape_delay_seconds": 8,
  "discord_webhook_url": "...",
  "tmdb_api_key_configured": true,    // Boolean, not the actual key
  "tmdb_auto_enrich": false,
  "tmdb_enrich_on_add": true
}
```

**Settings PUT Request:**
```json
{
  "tmdb_api_key": "abc123...",        // Only on write
  "tmdb_auto_enrich": true,
  "tmdb_enrich_on_add": true
}
```

#### 3.3 Update WebSocket Broadcasting
- [ ] Add message type `enrichment_progress`
- [ ] Add message type `enrichment_completed`
- [ ] Broadcast on individual enrichment completion
- [ ] Broadcast progress during bulk operations

**WebSocket Messages:**
```javascript
// Individual completion
{
  "type": "enrichment_completed",
  "item_id": 42,
  "item_type": "wishlist",
  "success": true,
  "tmdb_id": 550,
  "confidence": 0.95
}

// Bulk progress
{
  "type": "enrichment_progress",
  "processed": 5,
  "total": 10,
  "successful": 4,
  "failed": 1
}
```

#### 3.4 UI Updates
- [ ] Update `src/presentation/html_renderer.cpp`
- [ ] Add TMDb section to Settings page
- [ ] Add enrichment button (🎬 icon) to wishlist table rows
- [ ] Add enrichment button to collection table rows
- [ ] Add "Enrich All" button to page headers
- [ ] Add progress bar modal for bulk operations
- [ ] Update WebSocket handler to process enrichment messages

**Settings Page HTML:**
```html
<div class="settings-section">
    <h3>🎬 TMDb Integration</h3>
    <p class="settings-description">
        Automatically fetch movie metadata from The Movie Database.
        <a href="https://www.themoviedb.org/settings/api" target="_blank">Get your free API key</a>
    </p>

    <div class="form-group">
        <label for="settingsTmdbApiKey">API Key</label>
        <input type="password"
               id="settingsTmdbApiKey"
               placeholder="Enter your TMDb API v3 key"
               autocomplete="off">
        <small>Your API key is stored securely and never shared.</small>
    </div>

    <div class="form-group">
        <label>
            <input type="checkbox" id="settingsTmdbEnrichOnAdd">
            Auto-enrich when adding new items
        </label>
        <small>Automatically fetch metadata when adding movies to wishlist or collection</small>
    </div>
</div>
```

**Wishlist Table Enrichment Button:**
```html
<td class="actions">
    ${item.tmdb_id === 0 ? `
        <button onclick="enrichItem(${item.id}, 'wishlist')"
                class="btn-icon"
                title="Fetch TMDb metadata">
            🔍
        </button>
    ` : ''}
    <!-- Existing buttons: Edit, Delete, View History -->
</td>
```

**JavaScript Functions:**
```javascript
async function enrichItem(itemId, itemType) {
    const loadingToast = showToast('Fetching metadata...', 'info', 0);

    try {
        const response = await fetch(`/api/${itemType}/${itemId}/enrich`, {
            method: 'POST'
        });

        const result = await response.json();

        hideToast(loadingToast);

        if (result.success) {
            showToast(
                `Enriched with ${(result.confidence * 100).toFixed(0)}% confidence`,
                'success'
            );
            reloadCurrentPage();
        } else {
            showToast(`Failed: ${result.error}`, 'error');
        }
    } catch (error) {
        hideToast(loadingToast);
        showToast('Network error', 'error');
    }
}

async function bulkEnrichAll() {
    const confirmed = confirm(
        'Enrich all items without metadata? This may take several minutes for large collections.'
    );

    if (!confirmed) return;

    // Show progress modal
    showProgressModal('Enriching items...', 0, 100);

    const response = await fetch('/api/enrich/auto', { method: 'POST' });
    const result = await response.json();

    // Progress updates will come via WebSocket
}

// WebSocket handler
function handleWebSocketMessage(event) {
    const data = JSON.parse(event.data);

    switch (data.type) {
        case 'enrichment_progress':
            updateProgressModal(data.processed, data.total);
            break;

        case 'enrichment_completed':
            hideProgressModal();
            showToast(
                `Enrichment complete! ${data.successful} successful, ${data.failed} failed`,
                'success'
            );
            reloadCurrentPage();
            break;

        // ... other message types
    }
}
```

---

### Phase 4: Polish & Testing (Days 6-7)

**Goal**: Production-ready with comprehensive testing

#### 4.1 Error Handling
- [ ] Add try-catch blocks around all API calls
- [ ] Log all errors with context (item ID, title, error message)
- [ ] User-friendly error messages in UI
- [ ] Graceful degradation (app works without TMDb)

**Error Scenarios:**
1. No API key configured → "Please add your TMDb API key in Settings"
2. Invalid API key → "Invalid TMDb API key. Please check your configuration"
3. Network failure → "Network error. Please check your connection"
4. Rate limit exceeded → "Rate limit reached. Please try again in a few seconds"
5. No matches found → "Could not find a match. Try editing the title or adding manually"
6. Low confidence match → "Match confidence too low (X%). Please verify manually"

#### 4.2 Comprehensive Testing

**Manual Test Cases:**

| # | Test Case | Expected Result | Status |
|---|-----------|-----------------|--------|
| 1 | Add wishlist item "The Matrix (1999)" with auto-enrich enabled | TMDb data populated automatically | ⏸️ |
| 2 | Click enrich button on existing item | Metadata fetched and displayed | ⏸️ |
| 3 | Bulk enrich 20 items | Progress updates shown, all processed | ⏸️ |
| 4 | Enrich without API key | Clear error message shown | ⏸️ |
| 5 | Enrich with invalid API key | "Invalid API key" error | ⏸️ |
| 6 | Enrich obscure/typo title | "No match found" or low confidence | ⏸️ |
| 7 | Rapidly enrich 50 items | Rate limiting prevents 429 errors | ⏸️ |
| 8 | Disconnect network during enrich | "Network error" shown gracefully | ⏸️ |
| 9 | Edit enriched item manually | Manual edits preserved | ⏸️ |
| 10 | WebSocket disconnected during bulk | Progress still tracked, no crashes | ⏸️ |

**Known Movie Test Data:**
- "The Matrix (1999)" → tmdb_id: 603, imdb_id: tt0133093, rating: 8.1
- "Inception (2010)" → tmdb_id: 27205, imdb_id: tt1375666, rating: 8.3
- "The Shawshank Redemption (1994)" → tmdb_id: 278, imdb_id: tt0111161, rating: 8.7
- "Interstellar (2014)" → tmdb_id: 157336, imdb_id: tt0816692, rating: 8.3

**Edge Cases:**
- Title with special characters: "Amélie (2001)"
- Title with subtitle: "Dr. Strangelove or: How I Learned to Stop Worrying and Love the Bomb"
- Multiple movies with same title: "Halloween" (1978 vs 2018)
- Foreign title: "Le Fabuleux Destin d'Amélie Poulain"

#### 4.3 Performance Optimization
- [ ] Profile HTTP request times
- [ ] Optimize JSON parsing
- [ ] Add caching for repeated searches (optional)
- [ ] Minimize database transactions

**Performance Targets:**
- Single enrichment: < 2 seconds
- Bulk enrichment: ~250ms per item (rate limit constraint)
- UI remains responsive during background operations

#### 4.4 Documentation
- [ ] Update `CLAUDE.md` with TMDb integration details
- [ ] Add TMDb API setup instructions to README
- [ ] Document configuration options
- [ ] Add troubleshooting section

**CLAUDE.md Additions:**
```markdown
## TMDb Integration

### Configuration

1. Get your free API key from [TMDb](https://www.themoviedb.org/settings/api)
2. Navigate to Settings in the web UI
3. Paste your API key in the "TMDb API Key" field
4. Enable "Auto-enrich when adding new items" (optional)
5. Save settings

### Usage

**Automatic Enrichment:**
When enabled, new wishlist/collection items are automatically enriched with:
- TMDb ID
- IMDb ID
- TMDb rating
- YouTube trailer key

**Manual Enrichment:**
Click the 🔍 button next to any item to fetch metadata manually.

**Bulk Enrichment:**
Click "Enrich All" to process all items without metadata. Progress shown in real-time.

### Matching Algorithm

The enrichment service uses smart matching:
1. Normalize title (remove special characters, lowercase)
2. Extract year from title if present (e.g., "Movie (2023)")
3. Search TMDb with title + year filter
4. Score results: Title Similarity (70%) + Year Proximity (20%) + Popularity (10%)
5. Accept matches with confidence > 70%

**Confidence Levels:**
- 90-100%: Excellent match, very likely correct
- 70-89%: Good match, likely correct
- <70%: Low confidence, manual verification recommended

### Rate Limiting

TMDb free tier allows 40 requests per 10 seconds. The client enforces this with:
- Automatic delays (250ms between requests)
- Progress indicators for bulk operations
- Graceful handling of rate limit errors

### Troubleshooting

**"Please add your TMDb API key"**
→ Navigate to Settings and add your API v3 key from TMDb.

**"Could not find a match"**
→ Try editing the title to match TMDb exactly, or add metadata manually.

**"Rate limit reached"**
→ Wait a few seconds and try again. Bulk operations handle this automatically.

**"Network error"**
→ Check your internet connection and firewall settings.
```

---

## Build Configuration

### CMakeLists.txt Updates

```cmake
# Add new source files
set(SOURCES
    # ... existing sources ...

    # TMDb Integration
    src/infrastructure/tmdb_client.cpp
    src/application/enrichment/tmdb_enrichment_service.cpp
)
```

**No new dependencies required!** All needed libraries already present:
- libcurl (HTTP)
- nlohmann/json (JSON parsing)
- OpenSSL (HTTPS)

---

## Testing Strategy

### Unit Testing (Manual)

**TmdbClient Tests:**
```bash
# Create simple test in main.cpp with --test-tmdb flag
./bluray-tracker --test-tmdb --api-key YOUR_KEY

Test Cases:
1. Search "The Matrix" → Should find tmdb_id 603
2. Get details for 603 → Should return IMDb ID, rating
3. Get videos for 603 → Should return trailer key
4. Invalid API key → Should error gracefully
5. Network timeout → Should handle with retry
```

**TmdbEnrichmentService Tests:**
```bash
# Add items to database, then enrich
sqlite3 bluray-tracker.db "INSERT INTO wishlist (url, title, desired_max_price, source, created_at, last_checked) VALUES ('https://example.com', 'The Matrix (1999)', 19.99, 'amazon.nl', datetime('now'), datetime('now'));"

./bluray-tracker --enrich-all --db bluray-tracker.db

# Check results
sqlite3 bluray-tracker.db "SELECT title, tmdb_id, imdb_id, tmdb_rating FROM wishlist WHERE title LIKE '%Matrix%';"
```

### Integration Testing

**End-to-End Workflow:**
1. Start web server
2. Navigate to Settings → Add TMDb API key
3. Enable auto-enrich
4. Add new wishlist item "Inception"
5. Verify metadata populated automatically
6. Check logs for successful enrichment
7. Verify WebSocket broadcast received

**Bulk Enrichment Test:**
1. Add 20 items without metadata
2. Click "Enrich All" button
3. Observe progress bar updates
4. Verify all items enriched
5. Check timing: ~5 seconds for 20 items (250ms each)

### Stress Testing

**Rate Limit Test:**
```javascript
// Add 100 items and bulk enrich
for (let i = 0; i < 100; i++) {
    addWishlistItem(`Test Movie ${i}`);
}
bulkEnrichAll();

// Should take ~25 seconds (100 * 250ms)
// No 429 errors should occur
```

---

## Security Considerations

### API Key Protection

1. **Storage**: API key stored in SQLite database (not version control)
2. **Transmission**: Never returned in GET /api/settings responses
3. **UI**: Input type="password" to prevent shoulder surfing
4. **Logs**: API key redacted in all log messages

**Security Checklist:**
- [ ] API key never logged
- [ ] API key never returned in GET requests
- [ ] API key only accepted via HTTPS in production
- [ ] Rate limiting prevents abuse
- [ ] Database file permissions (600)

### Input Validation

All user inputs validated:
- Title: Max 500 characters, XSS prevention
- API key: Alphanumeric only, max 100 characters
- Item IDs: Integer validation, SQL injection prevention

---

## Migration Path

### For Existing Deployments

**Step 1: Deploy New Version**
```bash
# Build updated version
cmake --build build --config Release

# Stop existing container
docker-compose down

# Start new version
docker-compose up -d
```

**Step 2: Configure TMDb**
1. Navigate to Settings
2. Add TMDb API key
3. Leave auto-enrich disabled initially

**Step 3: Test with Sample Items**
1. Add 1-2 new items
2. Click "Enrich" button manually
3. Verify metadata appears correctly

**Step 4: Bulk Enrich Existing Items**
1. Click "Enrich All" button
2. Monitor progress (may take several minutes for large collections)
3. Review results

**Step 5: Enable Auto-Enrich**
1. Settings → Enable "Auto-enrich when adding new items"
2. Save settings
3. New items now enriched automatically

### Rollback Plan

If issues occur:
1. Revert to previous Docker image
2. Metadata fields remain (won't break)
3. Manual entry still works
4. No data loss

---

## Success Criteria

**Must Have (MVP):**
- ✅ Single item enrichment works via API
- ✅ TMDb API key configurable in Settings
- ✅ Rate limiting prevents 429 errors
- ✅ Basic error handling (network, API key, no matches)
- ✅ Manual enrichment button in UI

**Should Have:**
- ✅ Bulk enrichment for multiple items
- ✅ Progress indicators and WebSocket updates
- ✅ Auto-enrich on add (configurable)
- ✅ Confidence scoring for matches
- ✅ Smart title matching (fuzzy, year extraction)

**Nice to Have:**
- ⏸️ Enrichment logging table for analytics
- ⏸️ Caching layer for repeated searches
- ⏸️ CLI mode for batch enrichment
- ⏸️ Override protection (don't overwrite manual edits)

---

## Risk Mitigation

### Risk 1: Inaccurate Matching
**Mitigation:**
- Confidence threshold (>70%)
- Show confidence score to user
- Easy manual override
- Log all matches for review

### Risk 2: Rate Limiting Issues
**Mitigation:**
- Conservative rate (250ms delay)
- Sliding window tracking
- Progress indicators
- Resume capability (future)

### Risk 3: API Key Exposure
**Mitigation:**
- Never return in GET
- Password input type
- Database storage only
- Log redaction

### Risk 4: Network Failures
**Mitigation:**
- 10-second timeout
- Retry logic (1 retry)
- Clear error messages
- Graceful degradation

---

## Timeline

### Week 1: Core Implementation
- **Day 1-2**: TmdbClient + rate limiting
- **Day 3**: TmdbEnrichmentService + matching

### Week 2: Integration
- **Day 4**: API endpoints + Settings
- **Day 5**: UI updates + WebSocket

### Week 3: Polish
- **Day 6**: Bulk enrichment + testing
- **Day 7**: Documentation + deployment

**Total: 3 weeks (part-time) or 1 week (full-time)**

---

## Post-Launch

### Monitoring

Track these metrics:
- Enrichment success rate
- Average confidence scores
- Rate limit hits
- Network errors
- User adoption (% of items enriched)

### Future Enhancements

**Phase 2.1:**
- [ ] TMDb image downloads (posters, backdrops)
- [ ] Genre tagging from TMDb
- [ ] Cast/crew information
- [ ] Runtime and release date display

**Phase 2.2:**
- [ ] Enrichment logging table for analytics
- [ ] Automatic re-enrichment for low-confidence items
- [ ] Similarity-based recommendations
- [ ] "Similar Movies" feature

**Phase 3:**
- [ ] IMDb scraping for additional data
- [ ] Rotten Tomatoes integration
- [ ] User ratings vs. TMDb ratings comparison

---

## References

- [TMDb API v3 Documentation](https://developers.themoviedb.org/3)
- [TMDb API Rate Limits](https://developers.themoviedb.org/3/getting-started/request-rate-limiting)
- [Levenshtein Distance Algorithm](https://en.wikipedia.org/wiki/Levenshtein_distance)
- Existing codebase patterns: `NetworkClient`, `ConfigManager`, `DatabaseManager`

---

**Document Version**: 1.0
**Created**: 2026-01-22
**Author**: Claude (AI Assistant)
**Status**: Ready for Implementation

---

## Appendix: API Examples

### TMDb API v3 Endpoints

**Search Movie:**
```
GET https://api.themoviedb.org/3/search/movie?api_key=XXX&query=The%20Matrix&year=1999

Response:
{
  "page": 1,
  "results": [
    {
      "id": 603,
      "title": "The Matrix",
      "original_title": "The Matrix",
      "release_date": "1999-03-31",
      "vote_average": 8.1,
      "poster_path": "/f89U3ADr1oiB1s9GkdPOEpXUk5H.jpg",
      ...
    }
  ],
  "total_results": 1
}
```

**Get Movie Details:**
```
GET https://api.themoviedb.org/3/movie/603?api_key=XXX&append_to_response=external_ids

Response:
{
  "id": 603,
  "title": "The Matrix",
  "imdb_id": "tt0133093",
  "vote_average": 8.1,
  "runtime": 136,
  "external_ids": {
    "imdb_id": "tt0133093"
  },
  ...
}
```

**Get Movie Videos:**
```
GET https://api.themoviedb.org/3/movie/603/videos?api_key=XXX

Response:
{
  "results": [
    {
      "key": "m8e-FF8MsqU",
      "site": "YouTube",
      "type": "Trailer",
      "official": true,
      "name": "The Matrix (1999) Official Trailer"
    }
  ]
}
```

---

## Appendix: Code Snippets

### Title Normalization Function
```cpp
std::string TmdbMatchingStrategy::normalizeTitle(std::string_view title) {
    std::string normalized;
    normalized.reserve(title.size());

    // Convert to lowercase
    for (char c : title) {
        if (std::isalnum(c) || std::isspace(c)) {
            normalized += std::tolower(c);
        }
    }

    // Remove leading articles
    const std::vector<std::string> articles = {"the ", "a ", "an "};
    for (const auto& article : articles) {
        if (normalized.starts_with(article)) {
            normalized = normalized.substr(article.length());
            break;
        }
    }

    // Remove extra spaces
    normalized.erase(
        std::unique(normalized.begin(), normalized.end(),
            [](char a, char b) { return a == ' ' && b == ' '; }),
        normalized.end()
    );

    return normalized;
}
```

### Levenshtein Distance
```cpp
int levenshteinDistance(std::string_view s1, std::string_view s2) {
    const size_t m = s1.size();
    const size_t n = s2.size();

    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

    for (size_t i = 0; i <= m; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= n; ++j) dp[0][j] = j;

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
```

### Year Extraction
```cpp
int TmdbMatchingStrategy::extractYearFromTitle(std::string_view title) {
    // Match (YYYY) or [YYYY] at end of title
    std::regex year_regex(R"(.*[\(\[](\d{4})[\)\]].*$)");
    std::smatch match;
    std::string title_str(title);

    if (std::regex_match(title_str, match, year_regex)) {
        return std::stoi(match[1].str());
    }

    return 0;  // No year found
}
```

---

**End of Document**
