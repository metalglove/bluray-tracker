# Blu-ray Tracker - Feature Roadmap & Implementation Plan

**Version**: 1.1
**Date**: 2026-01-22
**Status**: Execution Phase - Tier 1 Complete

## Executive Summary

This roadmap organizes the 9 proposed feature enhancements into a phased implementation plan based on:
- **Technical feasibility** (existing infrastructure, dependencies)
- **User value** (impact on user experience)
- **Implementation effort** (development complexity, time investment)
- **Strategic fit** (alignment with project goals)

**Key Achievement**: Phase 1 foundation (Price History) and several usability enhancements (Collection Edit, Title Locking) are now **COMPLETED**. Additionally, all Tier 1 "Quick Wins" from FEATURES.md have been implemented (Ratings, Trailers, Tags, Bonus Features).

---

## Prioritization Matrix

### Scoring Criteria
- **Value**: 1 (Low) to 5 (High) - User impact and feature desirability
- **Effort**: 1 (Minimal) to 5 (Extensive) - Development complexity
- **Priority Score**: Value / Effort (higher is better)

| # | Feature | Value | Effort | Score | Phase | Status |
|---|---------|-------|--------|-------|-------|--------|
| 3 | **Price History Visualization** | 4 | 2 | **2.00** | 1 | ✅ **Completed** |
| 10 | **Release Calendar** | 4 | 3 | **1.33** | 1 | ✅ **Completed** |
| - | **TMDb/IMDb Ratings (Manual)** | 5 | 1 | **5.00** | 1 | ✅ **Completed** |
| - | **Movie Trailers** | 4 | 2 | **2.00** | 1 | ✅ **Completed** |
| - | **Custom Tags** | 3 | 2 | **1.50** | 1 | ✅ **Completed** |
| - | **Bonus Features Tracking** | 3 | 2 | **1.50** | 1 | ✅ **Completed** |
| 1 | **Automatic TMDb API Enrichment** | 5 | 3 | **1.67** | 2 | **Fields Ready** |
| 4 | **Multi-Site Expansion** | 4 | 3 | **1.33** | 2 | Extensible |
| 9 | **Custom Alert Rules** | 4 | 3 | **1.33** | 2 | Infrastructure Ready |
| 2 | **IMDb Discovery/Recommendations** | 4 | 4 | **1.00** | 2 | Depends on #1 |
| 6 | **Inventory Management (Barcode)** | 3 | 3 | **1.00** | 3 | UI Enhancement |
| 8 | **Backup & Cloud Sync** | 4 | 4 | **1.00** | 3 | Infrastructure |
| 5 | **Progressive Web App (PWA)** | 3 | 4 | **0.75** | 3 | Frontend Heavy |
| 7 | **Social Sharing (X/Twitter)** | 2 | 3 | **0.67** | 4 | Low Priority |


---



## Phase 1: Quick Wins & Foundations ✅ **COMPLETED**

### 1. Price History Visualization ⭐ **COMPLETED**

**Status**: **Live in v1.1**
**Effort**: LOW (Completed)
**Value**: HIGH

**Delivered**:
- ✅ Backend: Scheduler inserts records, API serves history JSON.
- ✅ Frontend: Chart.js integration with target price lines and interactive tooltips.
- ✅ Modal: "View History" button in Wishlist.

---

### 1b. Usability Enhancements ✅ **COMPLETED**
- ✅ **Wishlist Title Locking**: Prevent scraper overwrites.
- ✅ **Collection Editing**: Edit modal for collection capabilities.
- ✅ **Scraper Rate Limiting**: Throttling to prevent bans.
- ✅ **Debounced Search**: Responsive search for large collections.
- ✅ **Security Hardening**: Log sanitization and SQL injection prevention.
- ✅ **Bol.com Variant Fix**: Strict ID matching for 4K/BD accuracy.

---

### 1c. Tier 1 "Quick Wins" Features ✅ **COMPLETED**

**Status**: **Live in v1.1**
**Total Effort**: LOW-MEDIUM (Completed)
**Value**: VERY HIGH

**Delivered**:

#### Release Calendar Integration
- ✅ **Database**: release_calendar table with comprehensive fields
- ✅ **Repository**: Full CRUD operations with pagination
- ✅ **Scraper**: Blu-ray.com release calendar scraper
- ✅ **API**: `/api/release-calendar` endpoints (GET, POST, DELETE)
- ✅ **Frontend**: Dashboard integration with calendar grid view
- ✅ **CLI**: `--scrape-calendar` mode for scheduled updates
- ✅ **Auto-populate**: First-run automatic data fetch for polished UX

#### TMDb/IMDb Rating Display (Manual Entry)
- ✅ **Database**: tmdb_id, imdb_id, tmdb_rating columns added
- ✅ **API**: PUT endpoints support metadata updates
- ✅ **Frontend**: Color-coded rating badges (Green: 7+, Yellow: 5-7, Red: <5)
- ✅ **Edit Forms**: Fields for TMDb ID, IMDb ID, and rating
- ✅ **Display**: Rating shown on all wishlist and collection item cards

#### Movie Trailers Integration
- ✅ **Database**: trailer_key column for YouTube video keys
- ✅ **API**: PUT endpoints support trailer key updates
- ✅ **Frontend**: "Watch Trailer" button with 🎬 icon
- ✅ **Modal**: Embedded YouTube player with responsive iframe
- ✅ **Edit Forms**: Field for entering YouTube video keys

#### Custom Tags & Labels
- ✅ **Database**: tags table (id, name, color) and item_tags junction table
- ✅ **Repository**: Full CRUD with tag-item associations
- ✅ **API**: `/api/tags` endpoints (GET, POST, PUT, DELETE)
- ✅ **API**: Tag assignment endpoints for wishlist/collection items
- ✅ **Frontend**: Tag management modal accessible from sidebar
- ✅ **Frontend**: Colored tag pills displayed on items
- ✅ **UI**: Tag filtering and search capabilities

#### Bonus Features Tracking
- ✅ **Database**: edition_type, has_slipcover, has_digital_copy, bonus_features columns
- ✅ **API**: PUT endpoints support edition metadata updates
- ✅ **Frontend**: Edition type badges displayed on items
- ✅ **Edit Forms**: Fields for edition type, slipcover, digital copy, bonus features
- ✅ **Display**: Special edition indicators (Steelbook, Collector's, etc.)
 
---

### 2. Automatic TMDb API Enrichment ⭐ **NEXT PRIORITY**

**Status**: Manual entry complete, automatic API integration needed
**Effort**: MEDIUM (2-3 weeks)
**Value**: VERY HIGH (enables Feature 3 - Discovery/Recommendations)

**Current State**:
- ✅ Database fields exist: tmdb_id, imdb_id, tmdb_rating, trailer_key
- ✅ Frontend displays ratings, trailers, and metadata
- ✅ API supports updating metadata via PUT requests
- ⏳ **Missing**: Automatic TMDb API integration for enrichment

#### Architecture

**New Component**: `src/application/metadata/imdb_integrator.hpp/cpp`

```cpp
namespace bluray::application::metadata {

class ImdbIntegrator {
public:
    explicit ImdbIntegrator(std::string_view api_key);

    // Search by title, returns TMDb ID
    std::optional<int> searchByTitle(std::string_view title, int year = 0);

    // Enrich product/item with metadata
    void enrichProduct(domain::Product& product);
    void enrichWishlistItem(domain::WishlistItem& item);
    void enrichCollectionItem(domain::CollectionItem& item);

private:
    std::string api_key_;
    infrastructure::NetworkClient client_;

    struct MovieMetadata {
        std::string imdb_id;
        float rating;
        std::string genres;       // Comma-separated
        std::string plot;
        std::string poster_url;
        int runtime_minutes;
        std::string director;
    };

    std::optional<MovieMetadata> fetchMetadata(int tmdb_id);
    void cacheMetadata(const std::string& imdb_id, const MovieMetadata& data);
};

} // namespace
```

#### Implementation Tasks

**Database Schema Extension**
1. **Alter Tables** (add migration in `database_manager.cpp::createSchema()`)
   ```sql
   ALTER TABLE wishlist ADD COLUMN imdb_id TEXT;
   ALTER TABLE wishlist ADD COLUMN rating REAL;
   ALTER TABLE wishlist ADD COLUMN genres TEXT;
   ALTER TABLE wishlist ADD COLUMN plot TEXT;
   ALTER TABLE wishlist ADD COLUMN poster_url TEXT;

   ALTER TABLE collection ADD COLUMN imdb_id TEXT;
   ALTER TABLE collection ADD COLUMN rating REAL;
   ALTER TABLE collection ADD COLUMN genres TEXT;

   -- Cache table for API responses (reduce quota usage)
   CREATE TABLE IF NOT EXISTS imdb_cache (
       imdb_id TEXT PRIMARY KEY,
       metadata TEXT NOT NULL,  -- JSON blob
       cached_at TEXT NOT NULL,
       expires_at TEXT NOT NULL
   );
   ```

**Backend Implementation**
1. **API Integration** (use TMDb API - free tier: 1000 requests/day)
   - Register at https://www.themoviedb.org/settings/api
   - Store API key in `config` table: `INSERT INTO config VALUES ('tmdb_api_key', 'your_key')`
   - Endpoint: `https://api.themoviedb.org/3/search/movie?api_key={key}&query={title}`
   - Parse JSON response with nlohmann/json (already in project)

2. **NetworkClient Enhancement**
   - Add `std::optional<std::string> fetchJson(std::string_view url)` method
   - Handle rate limiting (429 status) with exponential backoff

3. **Enrichment Logic**
   - Extract year from title (regex: `(\\d{4})`)
   - Search TMDb with title + year
   - Fetch details: `/movie/{id}?api_key={key}&append_to_response=credits`
   - Parse: rating, genres, overview (plot), poster_path
   - Download poster via ImageCache (SHA256 of poster URL)
   - Update database record

4. **API Endpoints** (`web_frontend.cpp`)
   - `POST /api/wishlist/{id}/enrich` - Trigger manual enrichment
   - `POST /api/enrich-all` - Batch enrich all items (background task)
   - `GET /api/metadata/search?title=Inception` - Preview search results

**Frontend Enhancements**
1. **UI Elements**
   - "⚡ Enrich" button in wishlist/collection tables
   - Modal for enrichment preview before saving
   - Display in item cards:
     - ⭐ Rating badge (e.g., "8.8/10")
     - Genre tags (pill-shaped)
     - Plot summary (expandable)
     - High-res poster (replace scraped image)

2. **Settings Page**
   - Input field for TMDb API key
   - "Test Connection" button

**Error Handling**
- API failures: Log error, continue without enrichment
- No results: Suggest manual IMDB ID entry
- Cache expiration: 30 days, auto-refresh on access

**Testing**
- Mock API responses in tests
- Test with popular titles: "The Dark Knight (2008)", "Inception (2010)"
- Verify cache reduces API calls

**Success Metrics**
- ✅ 90%+ of wishlist items successfully enriched
- ✅ API quota usage <500 requests/day
- ✅ Enrichment completes in <3 seconds per item

---

## Phase 2: Enhanced Discovery & Alerts (2-4 Months)

### 3. IMDb Discovery/Recommendations Page

**Depends on**: Feature 1 (IMDb Metadata)
**Effort**: MEDIUM-HIGH (3-4 weeks)
**Value**: HIGH

#### Architecture

**New Route**: `/discover` in web_frontend.cpp

#### Implementation Tasks

1. **Recommendation Engine** (`src/application/metadata/recommendation_engine.hpp/cpp`)
   ```cpp
   class RecommendationEngine {
   public:
       struct Recommendation {
           std::string title;
           std::string imdb_id;
           float similarity_score;
           std::string reason;  // "Based on Inception (Sci-Fi, Thriller)"
           std::string poster_url;
           bool available_4k;   // Cross-check with scrapers
       };

       std::vector<Recommendation> getRecommendations(
           const std::vector<domain::CollectionItem>& collection,
           int limit = 20
       );
   };
   ```

2. **TMDb API Integration**
   - Use `/movie/{id}/recommendations` endpoint
   - Aggregate across all collection items
   - Score by: genre overlap, director match, rating proximity
   - Filter duplicates (already in collection/wishlist)

3. **Database Caching**
   - Table: `recommendations` (collection_item_id, suggested_imdb_id, score, cached_at)
   - Refresh weekly via cron job

4. **Availability Check**
   - For top 10 recommendations, search Amazon.nl/Bol.com
   - URL pattern: `https://www.amazon.nl/s?k={title}+blu-ray+4k`
   - Parse search results to find exact match
   - Cache availability status

5. **Frontend Page**
   - Grid layout (3 columns on desktop, 1 on mobile)
   - Card per recommendation:
     - Poster image
     - Title + Rating
     - "Why?" tooltip (genre/director match)
     - "In Stock" badge (if available)
     - "Add to Wishlist" button → Auto-fills add modal with URL
   - "Refresh Recommendations" button (POST /api/recommendations/refresh)

**Success Metrics**
- ✅ 20 personalized recommendations generated
- ✅ 60%+ of users find at least 1 interesting title
- ✅ 5% conversion to wishlist additions

---

### 4. Multi-Site Expansion (Coolblue, MediaMarkt)

**Effort**: MEDIUM (2-3 weeks per site)
**Value**: HIGH (regional relevance)

#### Implementation

1. **New Scrapers** (follow factory pattern in `scraper.cpp`)
   - `src/application/scraper/coolblue_nl_scraper.hpp/cpp`
   - `src/application/scraper/mediamarkt_nl_scraper.hpp/cpp`

2. **Site-Specific Parsing**
   - Coolblue: CSS selectors for price, stock, title, 4K indicator
   - MediaMarkt: Similar approach, handle JavaScript-rendered pages (may need headless browser)

3. **Factory Registration**
   ```cpp
   std::unique_ptr<IScraper> ScraperFactory::create(std::string_view url) {
       if (url.find("coolblue.nl") != std::string::npos) {
           return std::make_unique<CoolblueNlScraper>();
       }
       // ... existing scrapers
   }
   ```

4. **Configuration**
   - Add `scrape_coolblue` (bool), `scrape_mediamarkt` (bool) to config
   - Per-site delay settings

5. **Frontend Updates**
   - Source badges support new sites
   - Multi-site comparison table (if multiple URLs for same item)

**Challenges**
- Anti-scraping measures (Cloudflare, rate limits)
- Dynamic content (may need Playwright/Selenium)

**Workaround**: Use official APIs if available (check developer portals)

---

### 5. Release Calendar & Pre-order Tracker

**Effort**: MEDIUM (3 weeks)
**Value**: HIGH (planning tool)

#### Implementation

1.  **New Repository**: `src/infrastructure/repositories/release_calendar_repository.hpp/cpp`
    - Store upcoming releases with date, title, region, format (4K/BD).

2.  **Scraper Enhancement**
    - Scrape "Coming Soon" sections from supported sites.
    - Parse release dates (often found in product details: "Available on ...").

3.  **Frontend Calendar View**
    - New View: `/calendar`
    - FullCalendar.js or a lightweight custom CSS grid implementation.
    - Show releases as events on the calendar.
    - Click event -> "Add to Wishlist" / "Pre-order" link.

4.  **Notifications**
    - Notify user 1 week and 1 day before release of a wishlisted item.
    - "New Release" digest email.

**Success Metrics**
- ✅ Calendar accurately reflects 90% of major upcoming releases.
- ✅ Users add 5+ items to wishlist from calendar view per month.

---

### 6. Custom Alert Rules

**Effort**: MEDIUM (3 weeks)
**Value**: HIGH

#### Architecture

**New Component**: `src/application/alerts/rule_engine.hpp/cpp`

#### Implementation

1. **Rule Definition DSL** (JSON-based for simplicity)
   ```json
   {
       "name": "4K Deals Under €15",
       "conditions": [
           {"field": "is_uhd_4k", "operator": "equals", "value": true},
           {"field": "current_price", "operator": "less_than", "value": 15.0}
       ],
       "notify_via": ["discord", "email"]
   }
   ```

2. **Database Schema**
   ```sql
   CREATE TABLE alert_rules (
       id INTEGER PRIMARY KEY AUTOINCREMENT,
       name TEXT NOT NULL,
       conditions TEXT NOT NULL,  -- JSON
       notify_via TEXT NOT NULL,  -- Comma-separated: discord,email
       enabled INTEGER DEFAULT 1,
       created_at TEXT
   );
   ```

3. **Rule Evaluation** (in `Scheduler::runOnce()`)
   - After scraping, evaluate all enabled rules against updated wishlist items
   - Match conditions using simple expression evaluator
   - Trigger notifications for matching items

4. **Frontend**
   - Settings → "Alert Rules" tab
   - Visual rule builder (dropdowns for field, operator, value)
   - Preview: "3 items currently match this rule"

**Success Metrics**
- ✅ Users create 2+ custom rules on average
- ✅ Zero false positives in notifications

---

## Phase 3: Advanced UX & Infrastructure (4-6 Months)

### 7. Inventory Management (Barcode Scanning)

**Effort**: MEDIUM (3 weeks)
**Value**: MEDIUM-HIGH (for collectors)

#### Implementation

1. **Frontend** (JavaScript MediaDevices API)
   - Add "Scan Barcode" button in collection page
   - Use `navigator.mediaDevices.getUserMedia({video: true})`
   - Embed lightweight barcode library: [ZXing-JS](https://github.com/zxing-js/library) (50KB gzipped)
   - Decode EAN-13 barcodes from camera stream

2. **Barcode Lookup**
   - API: [Open Product Data](https://world.openfoodfacts.org/) or UPC Database
   - Search by UPC → Get title
   - Cross-reference with TMDb (search by title)
   - Auto-fill collection add form

3. **Fallback**: Manual UPC entry input

**Success Metrics**
- ✅ 80%+ barcode recognition accuracy
- ✅ <5 seconds from scan to auto-filled form

---

### 8. Backup & Cloud Sync

**Effort**: MEDIUM-HIGH (4 weeks)
**Value**: HIGH (data safety)

#### Implementation

1. **Backup Service** (`src/infrastructure/backup_service.hpp/cpp`)
   - SQLite `.backup` command via C API
   - Zip database + cache folder
   - Encrypt with AES-256 (use OpenSSL, already a dependency)

2. **Cloud Providers** (choose one initially)
   - **Dropbox**: OAuth 2.0 + file upload API
   - **Google Drive**: Similar OAuth flow
   - Store credentials encrypted in config table

3. **Scheduled Backups**
   - Add to docker-crontab: `0 2 * * * /app/bluray-tracker --backup`
   - New CLI flag: `--backup`

4. **Restore Flow**
   - Settings → "Restore from Cloud"
   - List available backups (with dates)
   - Download, decrypt, replace database
   - Restart server

**Security**
- Encryption key derived from user-set password (PBKDF2)
- Store key hash in config for validation

---

### 9. Progressive Web App (PWA)

**Effort**: HIGH (4-5 weeks)
**Value**: MEDIUM (convenience)

#### Implementation

1. **Service Worker** (embedded in web_frontend.cpp)
   - Cache static assets (HTML, CSS, JS, images)
   - Offline fallback page
   - Background sync for queue actions (add to wishlist while offline)

2. **Manifest** (JSON in web_frontend.cpp)
   ```json
   {
       "name": "Blu-ray Tracker",
       "short_name": "BluTracker",
       "start_url": "/",
       "display": "standalone",
       "theme_color": "#667eea",
       "icons": [{"src": "/icon-512.png", "sizes": "512x512"}]
   }
   ```

3. **Push Notifications** (Web Push API)
   - Generate VAPID keys (stored in config)
   - Subscribe clients via JS
   - Trigger from DiscordNotifier (also send web push)

4. **IndexedDB** (client-side storage for offline queue)

**Testing**
- Test on Android Chrome, iOS Safari
- Verify offline functionality

---

## Phase 4: Social & Polish (6+ Months)

### 10. Social Sharing (X/Twitter Integration)

**Effort**: MEDIUM (2-3 weeks)
**Value**: LOW-MEDIUM

#### Implementation

1. **Share Buttons** (JavaScript)
   - Generate share text: "Just added [Title] to my 4K collection! Check out my Blu-ray Tracker 📀"
   - X share URL: `https://twitter.com/intent/tweet?text={encoded_text}&url={app_url}`

2. **Auto-Tweet** (optional)
   - Use X API v2 with OAuth 1.0a
   - Store tokens in config
   - Post tweet on collection addition (user toggle)

3. **Public Profile Pages**
   - `/share/collection/{uuid}` - Read-only collection view
   - Generate shareable link in settings
   - Privacy: Toggle in settings (default: disabled)

**Considerations**
- X API rate limits (50 tweets/day free tier)
- Privacy concerns (don't auto-share without consent)

---

## Implementation Roadmap Summary

### Timeline

| Phase | Duration | Features | Key Deliverables |
|-------|----------|----------|------------------|
| **Phase 1** | Weeks 1-8 | Price History, IMDb Metadata | Visualization, Enriched UI |
| **Phase 2** | Weeks 9-16 | Recommendations, Multi-Site, Custom Alerts, Calendar | Discovery Page, New Scrapers, Rule Engine, Release Calendar |
| **Phase 3** | Weeks 17-24 | Inventory, Backup, PWA | Barcode Scanner, Cloud Sync, Mobile App |
| **Phase 4** | Weeks 25+ | Social Sharing | X Integration, Public Profiles |

### Resource Requirements

**Development**
- 1 C++ developer (backend)
- 1 frontend developer (JavaScript/CSS) - or same person
- Part-time: API integration, testing

**Infrastructure**
- TMDb API key (free tier)
- Optional: Cloud storage account (Dropbox/Google)
- Optional: X API developer account

**Testing**
- Manual testing on desktop/mobile
- Integration tests for new API endpoints
- Load testing for new scrapers

---

## Risk Assessment & Mitigation

### High-Risk Items

1. **API Rate Limits** (TMDb, UPC Database)
   - **Mitigation**: Aggressive caching (30-day expiration), batch processing
   - **Fallback**: Manual metadata entry

2. **Scraper Breakage** (site redesigns)
   - **Mitigation**: Implement site health checks, alert on failures
   - **Fallback**: Pause scraping until fixed

3. **Cloud Sync Security**
   - **Mitigation**: Strong encryption (AES-256), user-controlled keys
   - **Compliance**: GDPR considerations for EU users (North Brabant, NL)

4. **PWA Browser Compatibility**
   - **Mitigation**: Progressive enhancement, test on Safari/Chrome/Firefox
   - **Fallback**: Traditional web app still works

### Low-Risk Items
- Price history visualization (minimal code changes)
- UI enhancements (CSS/JS only, no breaking changes)

---

## Success Criteria

### Phase 1 (Weeks 1-8) ✅ **COMPLETED**
- ✅ Price history charts display for all wishlist items
- ✅ Release calendar fully functional with auto-population
- ✅ Manual metadata entry (ratings, trailers, tags, bonus features)
- ✅ Zero regression in existing functionality

**Completed Items:**
- Price History Visualization
- Release Calendar (Blu-ray.com integration)
- TMDb/IMDb Rating Display (manual entry)
- Movie Trailers Integration
- Custom Tags & Labels
- Bonus Features Tracking
- All usability enhancements (title locking, collection editing, etc.)

### Phase 2 (Weeks 9-16) - **IN PROGRESS**
- [ ] Automatic TMDb API enrichment (80%+ success rate)
- [ ] Discovery page shows 20+ personalized recommendations
- [ ] 2 new scrapers added (Coolblue, MediaMarkt)
- [ ] Custom alert rules functional with 95%+ accuracy

### Phase 3 (Weeks 17-24)

- [ ] Barcode scanning works on 5+ device types
- [ ] Automated cloud backups configured
- [ ] PWA installable on mobile devices

### Phase 4 (Weeks 25+)
- [ ] Social sharing generates 10+ shares per week
- [ ] Public profile pages accessible
- [ ] User satisfaction survey: 8/10+ rating

---

## Next Steps

### Immediate Actions (This Week)

1. **Confirm Priorities** with stakeholders
   - Review this roadmap
   - Adjust phase ordering if needed

2. **Set Up Phase 1 Development**
   - Create feature branch: `feature/price-history-viz`
   - Register TMDb API key
   - Assign tasks

3. **Prepare Testing Environment**
   - Clone production DB to test instance
   - Add test data (3 months of price history)

4. **Documentation**
   - Update CLAUDE.md with new architecture
   - Create API documentation for new endpoints

### Weekly Review Cadence
- **Sprint Planning**: Monday mornings
- **Progress Check**: Thursday afternoons
- **Demo**: Friday end-of-day

---

## Appendix: Technical Debt & Refactoring

While implementing new features, address these maintenance items:

1. **Unit Tests**: Add GoogleTest framework
   - Test scrapers with mock HTML
   - Test repositories with in-memory SQLite

2. **Logging**: Structured logging with JSON format
   - Easier to parse in monitoring tools

3. **Error Handling**: Consistent exception hierarchy
   - Create custom exceptions for domain errors

4. **Configuration**: Validate config values
   - Reject invalid SMTP ports, negative delays

5. **Database Migrations**: Version-controlled schema changes
   - Add migration system (e.g., `schema_version` in config)

---

**Document Version**: 1.1
**Last Updated**: 2026-01-22
**Maintained By**: Claude (AI Assistant)
**Review Cycle**: Bi-weekly
**Last Verification**: 2026-01-22 - Codebase audit complete, Phase 1 verified as complete

---

## Feedback & Iteration

This roadmap is a living document. Adjust priorities based on:
- User feedback (feature requests, bug reports)
- Technical discoveries (API limitations, performance issues)
- Market changes (new competitors, site shutdowns)

**Feedback Channels**:
- GitHub Issues: [metalglove/bluray-tracker/issues](https://github.com/metalglove/bluray-tracker/issues)
- Discussions: [metalglove/bluray-tracker/discussions](https://github.com/metalglove/bluray-tracker/discussions)

---

**END OF ROADMAP**
