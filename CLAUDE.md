# CLAUDE.md - AI Assistant Guide for Blu-ray Tracker

## Project Overview

**Blu-ray Tracker** is a modern C++17 application for tracking Blu-ray/UHD 4K movie prices and availability on Dutch e-commerce sites (Amazon.nl and Bol.com). The application periodically scrapes product information, detects meaningful changes (price drops, stock updates), and sends notifications via Discord webhooks or email.

### Key Features
- **Modern Web-based Dashboard** (Crow framework with embedded SPA)
  - Sleek, responsive UI with dark/light mode toggle
  - Real-time updates via WebSocket
  - Single-page application with client-side routing
  - Animated statistics and gradient cards
  - Modal dialogs and toast notifications
- Periodic scraping with configurable delays
- Wishlist management with price thresholds
- Personal collection tracking
- Discord webhook and SMTP email notifications
- Image caching with SHA256-based filenames
- SQLite database for persistence
- Docker deployment with cron scheduling

### Technology Stack
- **Language**: C++17 (concepts, ranges, designated initializers are partially supported or polyfilled)
- **Build System**: CMake 3.22+ with FetchContent
- **Web Framework**: Crow (header-only)
- **HTTP Client**: libcurl (system package)
- **HTML Parser**: gumbo-parser (system package)
- **Database**: SQLite3 (system package)
- **JSON**: nlohmann/json (header-only via FetchContent)
- **Formatting**: fmt (header-only via FetchContent, replaces std::format)

---

## Project Structure

```
bluray-tracker/
├── CMakeLists.txt                 # Root build configuration
├── Dockerfile                     # Multi-stage Docker build
├── docker-compose.yml             # Docker Compose configuration
├── docker-crontab                 # Cron schedule for periodic scraping
├── src/
│   ├── main.cpp                   # Entry point with mode selection
│   ├── domain/                    # Core business models
│   │   ├── models.hpp/cpp         # Product, WishlistItem, CollectionItem, ChangeEvent
│   │   └── change_detector.hpp   # Observer pattern for change detection
│   ├── infrastructure/            # Technical concerns
│   │   ├── logger.hpp/cpp         # Thread-safe logging (fmt::format)
│   │   ├── database_manager.hpp/cpp  # RAII SQLite wrapper
│   │   ├── config_manager.hpp/cpp    # Configuration from database
│   │   ├── network_client.hpp/cpp    # RAII libcurl wrapper
│   │   ├── tmdb_client.hpp/cpp       # TMDb API v3 client with rate limiting
│   │   ├── image_cache.hpp/cpp       # SHA256-based image caching
│   │   └── repositories/          # Repository pattern
│   │       ├── wishlist_repository.hpp/cpp
│   │       └── collection_repository.hpp/cpp
│   ├── application/               # Business logic orchestration
│   │   ├── scheduler.hpp/cpp      # Scraping orchestration
│   │   ├── scraper/               # Factory pattern scrapers
│   │   │   ├── scraper.hpp/cpp    # IScraper interface & factory
│   │   │   ├── amazon_nl_scraper.hpp/cpp
│   │   │   └── bol_com_scraper.hpp/cpp
│   │   ├── enrichment/            # TMDb metadata enrichment
│   │   │   └── tmdb_enrichment_service.hpp/cpp  # Smart matching & enrichment
│   │   └── notifier/              # Strategy + Observer pattern
│   │       ├── notifier.hpp       # INotifier interface
│   │       ├── discord_notifier.hpp/cpp
│   │       └── email_notifier.hpp/cpp
│   └── presentation/              # User interface
│       └── web_frontend.hpp/cpp   # Crow REST API + HTML dashboard
├── cache/                         # Runtime: cached product images
├── data/                          # Runtime: SQLite database
└── templates/                     # (Optional) Static HTML/CSS/JS assets
```

---

## Architecture & Design Patterns

### Clean Architecture Layers

1. **Domain Layer** (`src/domain/`)
   - Pure business logic with no external dependencies
   - Defines core models and change detection
   - **Pattern**: Observer (ChangeDetector notifies IChangeObserver implementations)

2. **Infrastructure Layer** (`src/infrastructure/`)
   - Technical implementations (database, network, filesystem)
   - **Patterns**:
     - Singleton (Logger, DatabaseManager, ConfigManager)
     - Repository (IWishlistRepository, ICollectionRepository)
     - RAII (Statement wrapper, NetworkClient, Transaction guard)

3. **Application Layer** (`src/application/`)
   - Orchestrates business workflows
   - **Patterns**:
     - Factory (ScraperFactory creates appropriate scraper for URL)
     - Strategy (INotifier with Discord/Email implementations)
     - Observer (Notifiers observe ChangeDetector)

4. **Presentation Layer** (`src/presentation/`)
   - Modern web interface using Crow framework
   - REST API + WebSocket support
   - Embedded HTML/CSS/JS Single-Page Application (SPA)
   - Real-time updates via WebSocket broadcasting

---

## Web Frontend UI

### Modern User Interface

The web frontend is a **sleek, modern single-page application** embedded directly in the C++ binary. It provides an intuitive, responsive interface for managing your Blu-ray collection and wishlist.

#### Design Features

**Visual Design**
- Purple/indigo gradient color scheme (`#667eea` to `#764ba2`)
- Dark mode (default) with light mode toggle
- Smooth CSS transitions and animations
- Card-based layouts with elegant hover effects
- Modern Inter font family with system fallbacks
- Professional shadows and backdrop blur effects

**UI Components**
- Animated gradient statistics cards
- Modal dialogs for adding/editing items
- Toast notifications for user feedback
- Loading spinners for async operations
- Empty states with helpful call-to-action prompts
- Pagination controls with page numbers
- Connection status indicator with pulse animation

**Responsive Design**
- Mobile-friendly with collapsible sidebar
- Touch-optimized buttons and forms
- Adaptive grid layouts
- Font size scaling for small screens

#### Application Structure

**Dashboard Page**
- 4 animated statistics cards:
  - Wishlist items count
  - Collection items count
  - In-stock items count
  - UHD 4K items count
- Quick actions section:
  - Add to Wishlist button
  - Add to Collection button
  - Manual scrape trigger

**Wishlist Management**
- Paginated table view with product images
- Current price vs. target price comparison
- Stock status badges (In Stock / Out of Stock)
- Format badges (UHD 4K / Blu-ray)
- Source identification (Amazon.nl / Bol.com)
- Edit and delete actions per item
- Search bar and filter dropdowns (UI ready)
- Real-time updates when prices/stock change

**Collection Management**
- Similar paginated table view
- Purchase price display
- Notes field for each item
- Quick add and delete functionality
- Empty state with guidance

**Settings Page**
- Configure scrape delay (seconds)
- Discord webhook URL management
- Complete SMTP configuration:
  - Server, port, user credentials
  - From and To email addresses
- Save functionality with success feedback

#### Real-Time Updates (WebSocket)

The application uses WebSocket connections for real-time updates:

**Connection Management**
- Automatic connection on page load
- Connection status indicator in header
- Automatic reconnection on disconnect (5-second delay)
- Thread-safe broadcast to all connected clients

**Live Update Events**
- `wishlist_added` - New item added to wishlist
- `wishlist_updated` - Wishlist item edited
- `wishlist_deleted` - Wishlist item removed
- `collection_added` - New item added to collection
- `collection_deleted` - Collection item removed
- `scrape_completed` - Scraping finished with item count

**User Experience**
- Toast notifications appear for all updates
- Tables automatically refresh with new data
- Dashboard statistics update in real-time
- No page reloads required

#### Theme System

Users can toggle between dark and light modes:

**Dark Mode (Default)**
- Background: `#0f172a` (slate-900)
- Cards: `#1e293b` (slate-800)
- Text: `#f1f5f9` (slate-100)
- Borders: `#334155` (slate-700)

**Light Mode**
- Background: `#f8fafc` (slate-50)
- Cards: `#ffffff` (white)
- Text: `#0f172a` (slate-900)
- Borders: `#e2e8f0` (slate-200)

Theme preference is saved to `localStorage` and persists across sessions.

### Key Design Decisions

#### Configuration Storage
- **All configuration stored in SQLite** (table: `config`)
- No external config files (YAML, JSON, etc.)
- Default values inserted on first database creation
- Thread-safe access via ConfigManager singleton

#### Image Caching
- Images downloaded during scraping
- Stored with SHA256(url) + extension as filename
- Served via `/cache/{filename}` route
- Prevents duplicate downloads

#### Change Detection
- ChangeDetector compares old vs new WishlistItem states
- Notifies observers only for actionable events:
  - Price dropped below threshold
  - Item back in stock
- Price changes are logged but don't trigger notifications (unless threshold crossed)

---

## Database Schema

### `config` Table
```sql
CREATE TABLE config (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);
```

**Key Configuration Keys**:
- `scrape_delay_seconds` (default: 8)
- `discord_webhook_url`
- `smtp_server`, `smtp_port`, `smtp_user`, `smtp_pass`, `smtp_from`, `smtp_to`
- `web_port` (default: 8080)
- `cache_directory` (default: ./cache)
- `log_file`, `log_level`

### `wishlist` Table
```sql
CREATE TABLE wishlist (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    url TEXT NOT NULL UNIQUE,
    title TEXT NOT NULL,
    current_price REAL NOT NULL DEFAULT 0.0,
    desired_max_price REAL NOT NULL,
    in_stock INTEGER NOT NULL DEFAULT 0,
    is_uhd_4k INTEGER NOT NULL DEFAULT 0,
    image_url TEXT,
    local_image_path TEXT,
    source TEXT NOT NULL,
    notify_on_price_drop INTEGER NOT NULL DEFAULT 1,
    notify_on_stock INTEGER NOT NULL DEFAULT 1,
    created_at TEXT NOT NULL,
    last_checked TEXT NOT NULL
);
```

### `collection` Table
```sql
CREATE TABLE collection (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    url TEXT NOT NULL UNIQUE,
    title TEXT NOT NULL,
    purchase_price REAL NOT NULL DEFAULT 0.0,
    is_uhd_4k INTEGER NOT NULL DEFAULT 0,
    image_url TEXT,
    local_image_path TEXT,
    source TEXT NOT NULL,
    notes TEXT,
    purchased_at TEXT NOT NULL,
    added_at TEXT NOT NULL
);
```

### `price_history` Table
```sql
CREATE TABLE price_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    wishlist_id INTEGER NOT NULL,
    price REAL NOT NULL,
    in_stock INTEGER NOT NULL,
    recorded_at TEXT NOT NULL,
    FOREIGN KEY (wishlist_id) REFERENCES wishlist(id) ON DELETE CASCADE
);
```

---

## Development Workflows

### Building the Project

#### Prerequisites (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake git \
    libcurl4-openssl-dev \
    libgumbo-dev \
    libsqlite3-dev \
    libssl-dev
```

#### Build Commands
```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release -j$(nproc)

# Install (optional)
cmake --install build --prefix /usr/local
```

#### CMake Build Options
- `CMAKE_BUILD_TYPE`: `Debug` or `Release`
- `CMAKE_CXX_COMPILER`: Specify compiler (g++, clang++)

### Running the Application

#### Web Server Mode (default)
```bash
./bluray-tracker --run --port 8080 --db ./bluray-tracker.db
```

Access dashboard at `http://localhost:8080`

**Automatic Initialization**: On first startup, if the release calendar is empty, the application automatically fetches initial data from blu-ray.com before starting the web server. This ensures a polished first-user experience without requiring manual scraping.

#### Scraper Modes (one-time execution)

**Wishlist Scraper** - Checks prices and stock on Amazon.nl & Bol.com
```bash
./bluray-tracker --scrape --db ./bluray-tracker.db
```
Use in cron jobs for frequent price monitoring (recommended: every 6 hours).

**Release Calendar Scraper** - Fetches upcoming releases from blu-ray.com
```bash
./bluray-tracker --scrape-calendar --db ./bluray-tracker.db
```
Use in cron jobs for periodic calendar updates (recommended: once daily).

#### Docker Deployment
```bash
# Build and run with docker-compose
docker-compose up -d

# View logs
docker-compose logs -f

# Stop
docker-compose down
```

### Adding a New Scraper

1. **Create header file** `src/application/scraper/newsite_scraper.hpp`:
```cpp
#pragma once
#include "scraper.hpp"

class NewSiteScraper : public IScraper {
public:
    std::optional<domain::Product> scrape(std::string_view url) override;
    bool canHandle(std::string_view url) const override;
    std::string_view getSource() const override { return "newsite.nl"; }
};
```

2. **Implement scraping logic** in `.cpp` file
3. **Register in ScraperFactory** (`src/application/scraper/scraper.cpp`):
```cpp
std::unique_ptr<IScraper> ScraperFactory::create(std::string_view url) {
    auto newsite = std::make_unique<NewSiteScraper>();
    if (newsite->canHandle(url)) {
        return newsite;
    }
    // ... existing scrapers
}
```

4. **Update CMakeLists.txt** to include new source files

### Adding a New Notifier

1. **Create implementation** inheriting from `INotifier`
2. **Register in main.cpp**:
```cpp
auto new_notifier = std::make_shared<NewNotifier>();
scheduler->addNotifier(new_notifier);
```

---

## Code Conventions

### Modern C++17 and fmt
 
#### Use fmt::format for string formatting
```cpp
std::string message = fmt::format("Price: €{:.2f}", price);
Logger::instance().info(message);
```

#### Designated initializers for structs
```cpp
domain::Product product{
    .url = url,
    .title = title,
    .price = 12.99,
    .in_stock = true
};
```

#### std::source_location for logging
```cpp
void debug(std::string_view message,
           const std::source_location& location = std::source_location::current());
```

#### RAII everywhere
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Wrap C resources (CURL*, sqlite3*, FILE*)
- Transaction guards for database operations

### Error Handling

- **Exceptions** for unrecoverable errors (database init failure, etc.)
- **std::optional** for operations that may legitimately fail (scraping, cache lookup)
- **Log all errors** with context using Logger

### Thread Safety

- **Mutex protection** for shared resources:
  - DatabaseManager (std::mutex for sqlite3 access)
  - Logger (std::mutex for file writes)
  - ConfigManager (std::mutex for config cache)
- Use `std::lock_guard` or `std::unique_lock`

### Naming Conventions

- **Classes**: PascalCase (`WishlistRepository`)
- **Functions**: camelCase (`findById`, `addNotifier`)
- **Variables**: snake_case (`scrape_delay_seconds_`)
- **Member variables**: trailing underscore (`client_`, `mutex_`)
- **Constants**: UPPER_SNAKE_CASE (rare, prefer constexpr)

---

## Testing & Debugging

### Logging

Set log level in database:
```sql
UPDATE config SET value = 'debug' WHERE key = 'log_level';
```

Or programmatically:
```cpp
Logger::instance().setLevel(LogLevel::Debug);
```

### Manual Scraping Test

```bash
# Add test item to wishlist
sqlite3 bluray-tracker.db "INSERT INTO wishlist (url, title, desired_max_price, source, created_at, last_checked) VALUES ('https://www.amazon.nl/dp/B0EXAMPLE', 'Test Movie', 19.99, 'amazon.nl', datetime('now'), datetime('now'));"

# Run scraper
./bluray-tracker --scrape --db bluray-tracker.db
```

### Debugging Scrapers

1. Enable debug logging
2. Check HTML structure of target site
3. Update CSS selectors / element IDs in scraper
4. Test with actual product URLs

### Common Issues

**Issue**: Scraping fails with 403/429 errors
**Solution**: Increase `scrape_delay_seconds` in config

**Issue**: Images not loading
**Solution**: Check `cache_directory` path and permissions

**Issue**: Notifications not sent
**Solution**: Verify Discord webhook URL or SMTP credentials in config

---

## TMDb Integration

### Overview

The Blu-ray Tracker features **automatic metadata enrichment** via The Movie Database (TMDb) API v3. This allows you to automatically fetch movie metadata including TMDb IDs, IMDb IDs, ratings, and YouTube trailer keys.

### Features

- **Automatic Enrichment**: Fetch metadata with a single click
- **Smart Matching**: Fuzzy title matching with confidence scoring
- **Batch Operations**: Enrich multiple items at once
- **Real-time Progress**: WebSocket updates during bulk enrichment
- **Manual Override**: All metadata fields remain manually editable
- **Secure Configuration**: API keys stored encrypted, never exposed in API responses

### Architecture

**Infrastructure Layer** (`src/infrastructure/`)
- `tmdb_client.hpp/cpp` - HTTP client for TMDb API v3 with rate limiting

**Application Layer** (`src/application/enrichment/`)
- `tmdb_enrichment_service.hpp/cpp` - Enrichment orchestration and smart matching

**Key Design Patterns**:
- **Levenshtein Distance Algorithm**: Fuzzy title matching
- **Rate Limiting**: 40 requests per 10 seconds (TMDb free tier)
- **Thread-safe Operations**: Mutex-protected bulk enrichment
- **Confidence Scoring**: Multi-factor confidence calculation

### Configuration

#### Getting a TMDb API Key

1. Create a free account at [TMDb](https://www.themoviedb.org/signup)
2. Navigate to [API Settings](https://www.themoviedb.org/settings/api)
3. Request an API key (v3)
4. Copy your API key

#### Setting Up in the Application

**Via Web UI** (Recommended):
1. Navigate to **Settings** page (⚙️ icon in sidebar)
2. Scroll to **TMDb Integration** section
3. Paste your API key in the **TMDb API Key (v3)** field
4. (Optional) Enable **Auto-enrich when adding new items**
5. Click **Save Settings**

**Via Database**:
```sql
UPDATE config SET value='your_api_key_here' WHERE key='tmdb_api_key';
UPDATE config SET value='1' WHERE key='tmdb_enrich_on_add';
```

**Configuration Keys**:
- `tmdb_api_key` - Your TMDb API v3 key (default: empty)
- `tmdb_auto_enrich` - Enable bulk auto-enrichment (default: 0)
- `tmdb_enrich_on_add` - Auto-enrich new items (default: 1)

### Usage

#### Manual Enrichment (Single Item)

**From Wishlist or Collection Table**:
1. Locate an item without metadata (missing rating/trailer)
2. Click the **🔍 Fetch TMDb metadata** button
3. Wait for enrichment to complete (~1-2 seconds)
4. View updated metadata (rating badge, trailer button)

**Confidence Score**:
- ✅ **90-100%**: Excellent match, very likely correct
- ⚠️ **70-89%**: Good match, likely correct
- ❌ **<70%**: Low confidence, manual verification recommended

**What Gets Enriched**:
- **TMDb ID**: Unique identifier for the movie
- **IMDb ID**: Cross-reference ID (e.g., "tt0133093")
- **TMDb Rating**: Average user rating (0.0-10.0)
- **Trailer Key**: YouTube video ID for trailer

#### Bulk Enrichment

**Auto-Enrich All Unenriched Items**:
```bash
# Via API
curl -X POST http://localhost:8080/api/enrich/auto \
  -H "Content-Type: application/json" \
  -d '{"item_type": "wishlist"}'

# For collection items
curl -X POST http://localhost:8080/api/enrich/auto \
  -H "Content-Type: application/json" \
  -d '{"item_type": "collection"}'
```

**Progress Tracking**:
- Real-time WebSocket broadcasts during enrichment
- Toast notifications show completion status
- Dashboard automatically refreshes with new metadata

**Estimated Times**:
- Single item: ~1-2 seconds
- 10 items: ~3 seconds (250ms delay between requests)
- 100 items: ~25 seconds
- 1000 items: ~4 minutes

### Smart Matching Algorithm

The enrichment service uses a sophisticated matching algorithm to ensure accuracy:

#### Title Normalization

```cpp
normalizeTitle("The Matrix (1999)") → "matrix"
```

**Steps**:
1. Convert to lowercase
2. Remove special characters
3. Remove leading articles ("the", "a", "an")
4. Remove year patterns (YYYY)
5. Trim whitespace

#### Confidence Calculation

```
Confidence = (Title Similarity × 0.7) + (Year Proximity × 0.2) + (Popularity × 0.1)
```

**Title Similarity (70% weight)**:
- Levenshtein distance algorithm
- Checks both `title` and `original_title` from TMDb
- Perfect match = 1.0, decreases with edit distance

**Year Proximity (20% weight)**:
- Extracts year from title using regex: `(YYYY)` or `[YYYY]`
- Perfect match (same year) = 1.0
- ±1 year = 0.8
- ±2-3 years = 0.5
- >3 years = 0.2

**Popularity (10% weight)**:
- TMDb `vote_average` normalized to 0-1 scale
- Helps differentiate between remakes/sequels

#### Matching Threshold

- **Minimum Confidence**: 0.7 (70%)
- Below threshold: Enrichment fails with "No confident match found"
- User can manually edit metadata if automatic matching fails

#### Example Matches

| Original Title | TMDb Match | Confidence | Result |
|----------------|------------|------------|---------|
| "The Matrix (1999)" | "The Matrix" | 1.00 | ✅ Perfect |
| "Inception" | "Inception" | 0.95 | ✅ Excellent |
| "Star Wars" | "Star Wars: Episode IV - A New Hope" | 0.82 | ✅ Good |
| "Movie" | "Movie Title 2023" | 0.45 | ❌ Too Low |

### Rate Limiting

**TMDb Free Tier Limits**:
- **40 requests per 10 seconds**
- Our implementation: **Conservative 250ms delay** (4 req/sec)

**Implementation**:
```cpp
// Sliding window rate limiting in TmdbClient
bool checkRateLimit() {
    auto elapsed = now - window_start;
    if (elapsed >= 10s) {
        window_start = now;
        requests_made = 0;
    }
    return requests_made < 40;
}
```

**Benefits**:
- Never exceeds TMDb limits
- No 429 (Too Many Requests) errors
- Consistent performance

### Trailer Selection

Trailers are selected with the following priority:

1. **Official Trailer** (YouTube, official=true, type="Trailer")
2. **Trailer** (YouTube, type="Trailer")
3. **Official Teaser** (YouTube, official=true, type="Teaser")
4. **Teaser** (YouTube, type="Teaser")
5. **First YouTube Video** (fallback)

Only YouTube videos are supported. Trailer keys are stored as YouTube video IDs (e.g., "dQw4w9WgXcQ").

### API Endpoints

#### Enrich Single Item

**`POST /api/wishlist/{id}/enrich`**
**`POST /api/collection/{id}/enrich`**

**Request**: Empty body

**Response (Success)**:
```json
{
  "success": true,
  "tmdb_id": 603,
  "imdb_id": "tt0133093",
  "tmdb_rating": 8.1,
  "trailer_key": "m8e-FF8MsqU",
  "confidence": 0.95
}
```

**Response (Failure)**:
```json
{
  "success": false,
  "error": "No confident match found (best confidence below 70%)",
  "confidence": 0.65
}
```

**WebSocket Broadcast**: `wishlist_updated` or `collection_updated`

#### Bulk Enrichment

**`POST /api/enrich/bulk`**

**Request**:
```json
{
  "item_ids": [1, 2, 3, 4, 5],
  "item_type": "wishlist"
}
```

**Response**:
```json
{
  "started": true,
  "total": 5
}
```

**WebSocket Broadcasts**:
- `enrichment_progress` - Periodic updates during processing
- `enrichment_completed` - Final results

**Progress Message Example**:
```json
{
  "type": "enrichment_progress",
  "processed": 3,
  "total": 5,
  "successful": 2,
  "failed": 1,
  "is_active": true
}
```

#### Auto-Enrich All

**`POST /api/enrich/auto`**

Enriches all items where `tmdb_id = 0`.

**Request**:
```json
{
  "item_type": "wishlist"  // or "collection"
}
```

**Response**:
```json
{
  "started": true,
  "total": 42
}
```

#### Get Progress

**`GET /api/enrich/progress`**

**Response**:
```json
{
  "total": 10,
  "processed": 7,
  "successful": 6,
  "failed": 1,
  "is_active": true,
  "current_item_id": 42
}
```

### Error Handling

**Common Errors**:

| Error | Cause | Solution |
|-------|-------|----------|
| "TMDb API key not configured" | No API key set | Add API key in Settings |
| "No confident match found" | Low confidence score | Edit title for better matching or add manually |
| "Network error" | Connection failure | Check internet connection |
| "Rate limit exceeded" | Too many requests | Wait 10 seconds and retry |
| "Invalid TMDb API key" | Wrong or expired key | Verify API key in TMDb settings |

**Logging**:

All enrichment operations are logged:

```
[INFO] Enriching wishlist item 42 ('The Matrix')
[DEBUG] TMDb search for 'The Matrix' (year hint: 1999)
[DEBUG] TMDb match candidate: 'The Matrix' (1999) - confidence: 1.00
[INFO] Selected TMDb match: 'The Matrix' (1999) with confidence 1.00
[INFO] Successfully enriched item 42 with TMDb ID 603 (confidence: 1.00)
```

Error logs include context:

```
[ERROR] TMDb enrichment failed for item 42 (title: 'Unknown Movie'): No TMDb results found for this title
```

### Database Schema

Metadata fields are already present in `wishlist` and `collection` tables:

```sql
-- Existing columns (added via migrations)
ALTER TABLE wishlist ADD COLUMN tmdb_id INTEGER DEFAULT 0;
ALTER TABLE wishlist ADD COLUMN imdb_id TEXT DEFAULT '';
ALTER TABLE wishlist ADD COLUMN tmdb_rating REAL DEFAULT 0.0;
ALTER TABLE wishlist ADD COLUMN trailer_key TEXT DEFAULT '';

-- Same columns for collection table
ALTER TABLE collection ADD COLUMN tmdb_id INTEGER DEFAULT 0;
ALTER TABLE collection ADD COLUMN imdb_id TEXT DEFAULT '';
ALTER TABLE collection ADD COLUMN tmdb_rating REAL DEFAULT 0.0;
ALTER TABLE collection ADD COLUMN trailer_key TEXT DEFAULT '';
```

### Security

**API Key Protection**:
- Stored in database, never in code or version control
- **Never returned** in `GET /api/settings` responses
- Only `tmdb_api_key_configured` boolean is exposed
- Password field type in UI prevents shoulder surfing
- Only sent via `PUT /api/settings` when explicitly changed

**Input Validation**:
- All user inputs sanitized before TMDb API calls
- URL encoding for query parameters
- XSS prevention in all UI displays

### Performance

**Caching**:
- TMDb responses are NOT cached (real-time data)
- Database updates are immediate
- WebSocket broadcasts ensure all clients see updates

**Optimization**:
- Bulk operations use single database transaction
- Rate limiting prevents API throttling
- Progress tracking doesn't block UI

**Benchmarks** (100 items):
- Enrichment time: ~25 seconds
- Success rate: ~85% (depends on title accuracy)
- Average confidence: ~0.87
- Network overhead: ~2MB total

### Troubleshooting

**"No matches found"**:
1. Check title spelling
2. Try adding year: "Movie Title (2023)"
3. Check if movie exists on TMDb
4. Consider using IMDb ID if available

**Low confidence scores**:
1. Verify title matches TMDb exactly
2. Add release year to title
3. Check for international vs. English title differences
4. Manually edit metadata if automatic matching fails

**Rate limiting**:
- Automatic delays prevent this
- If encountered, wait 10 seconds
- Check for other applications using same API key

**API key issues**:
1. Verify key is for API v3 (not v4)
2. Check key hasn't been revoked
3. Ensure key has proper permissions
4. Regenerate key if necessary

---

## API Reference

### REST Endpoints

#### Wishlist

- **`GET /api/wishlist?page=1&size=20`** - List wishlist (paginated)

  Response:
  ```json
  {
    "items": [...],
    "page": 1,
    "page_size": 20,
    "total_count": 42,
    "total_pages": 3,
    "has_next": true,
    "has_previous": false
  }
  ```

  Each item includes:
  - `id`, `url`, `title`
  - `current_price`, `desired_max_price`
  - `in_stock`, `is_uhd_4k`
  - `image_url`, `local_image_path`, `source`
  - `notify_on_price_drop`, `notify_on_stock`
  - `created_at`, `last_checked` (formatted timestamps)

- **`POST /api/wishlist`** - Add item

  Request:
  ```json
  {
    "url": "https://www.amazon.nl/...",
    "title": "Movie Title",
    "desired_max_price": 19.99,
    "notify_on_price_drop": true,
    "notify_on_stock": true
  }
  ```

  Response: `201 Created` with item JSON

  **Note**: Source is auto-detected from URL (amazon.nl or bol.com)

  **WebSocket Event**: Broadcasts `wishlist_added` to all clients

- **`PUT /api/wishlist/{id}`** - Update item

  Request:
  ```json
  {
    "title": "Updated Title",
    "desired_max_price": 15.99,
    "notify_on_price_drop": true,
    "notify_on_stock": false
  }
  ```

  Response: `200 OK` with updated item JSON

  **WebSocket Event**: Broadcasts `wishlist_updated` to all clients

- **`DELETE /api/wishlist/{id}`** - Remove item

  Response: `200 OK` on success, `404 Not Found` if item doesn't exist

  **WebSocket Event**: Broadcasts `wishlist_deleted` with `id` to all clients

#### Collection

- **`GET /api/collection?page=1&size=20`** - List collection (paginated)

  Response format identical to wishlist endpoint

  Each item includes:
  - `id`, `url`, `title`
  - `purchase_price`, `is_uhd_4k`
  - `image_url`, `local_image_path`, `source`
  - `notes`
  - `purchased_at`, `added_at` (formatted timestamps)

- **`POST /api/collection`** - Add item

  Request:
  ```json
  {
    "url": "https://www.bol.com/...",
    "title": "Movie Title",
    "purchase_price": 15.99,
    "is_uhd_4k": true,
    "notes": "Purchased on sale"
  }
  ```

  Response: `201 Created` with item JSON

  **WebSocket Event**: Broadcasts `collection_added` to all clients

- **`DELETE /api/collection/{id}`** - Remove item

  Response: `200 OK` on success, `404 Not Found` if item doesn't exist

  **WebSocket Event**: Broadcasts `collection_deleted` with `id` to all clients

#### Dashboard & Statistics

- **`GET /api/stats`** - Get dashboard statistics

  Response:
  ```json
  {
    "wishlist_count": 42,
    "collection_count": 87,
    "in_stock_count": 15,
    "uhd_4k_count": 23
  }
  ```

#### Actions

- **`POST /api/scrape`** - Trigger manual scrape

  Response on success:
  ```json
  { "success": true, "processed": 15 }
  ```

  Response on failure:
  ```json
  { "success": false, "error": "Error message" }
  ```

  **WebSocket Event**: Broadcasts `scrape_completed` with `processed` count to all clients

#### Settings

- **`GET /api/settings`** - Get configuration

  Response:
  ```json
  {
    "scrape_delay_seconds": 8,
    "discord_webhook_url": "https://discord.com/api/webhooks/...",
    "smtp_server": "smtp.gmail.com",
    "smtp_port": "587",
    "smtp_user": "user@gmail.com",
    "smtp_from": "from@example.com",
    "smtp_to": "to@example.com",
    "web_port": "8080",
    "cache_directory": "./cache"
  }
  ```

  **Note**: `smtp_pass` is never returned in GET requests

- **`PUT /api/settings`** - Update configuration

  Request (all fields optional):
  ```json
  {
    "scrape_delay_seconds": 10,
    "discord_webhook_url": "https://...",
    "smtp_server": "smtp.gmail.com",
    "smtp_port": "587",
    "smtp_user": "user@gmail.com",
    "smtp_pass": "password",
    "smtp_from": "from@example.com",
    "smtp_to": "to@example.com"
  }
  ```

  Response: `200 OK` with "Settings updated" message

#### Static Files

- **`GET /cache/{filename}`** - Serve cached product image

  Response: Image file (JPEG, PNG, GIF, WebP)

  Headers:
  - `Content-Type`: Appropriate image MIME type
  - `Cache-Control`: `public, max-age=31536000` (1 year)

---

### WebSocket API

#### Connection

- **Endpoint**: `WS /ws`
- **Protocol**: `ws://` or `wss://` (follows HTTP/HTTPS)

#### Connection Lifecycle

**Client connects:**
```javascript
const ws = new WebSocket('ws://localhost:8080/ws');

ws.onopen = () => {
  console.log('Connected to live updates');
};

ws.onclose = () => {
  console.log('Disconnected');
  // Implement auto-reconnect
  setTimeout(initWebSocket, 5000);
};

ws.onerror = (error) => {
  console.error('WebSocket error:', error);
};

ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  handleUpdate(data);
};
```

#### Message Format

All messages are JSON-encoded with a `type` field:

**Wishlist Events:**
```json
{
  "type": "wishlist_added",
  "item": { /* WishlistItem JSON */ }
}
```

```json
{
  "type": "wishlist_updated",
  "item": { /* Updated WishlistItem JSON */ }
}
```

```json
{
  "type": "wishlist_deleted",
  "id": 42
}
```

**Collection Events:**
```json
{
  "type": "collection_added",
  "item": { /* CollectionItem JSON */ }
}
```

```json
{
  "type": "collection_deleted",
  "id": 87
}
```

**Scraping Events:**
```json
{
  "type": "scrape_completed",
  "processed": 15
}
```

#### Broadcasting Behavior

- **Thread-safe**: Uses `std::mutex` to protect connection list
- **Broadcast to all**: When any client performs an action (add/edit/delete), all connected clients receive the update
- **Server-initiated**: Server broadcasts when scheduled scraping detects changes

#### Usage Example

```javascript
function handleUpdate(data) {
  switch (data.type) {
    case 'wishlist_updated':
      // Refresh wishlist table
      loadWishlist(currentPage);
      showToast('Wishlist updated', 'info');
      break;
    case 'scrape_completed':
      showToast(`Scrape completed! Processed ${data.processed} items`, 'success');
      loadDashboardStats();
      break;
    // ... handle other event types
  }
}
```

---

## Deployment Considerations

### Production Checklist

- [ ] Set strong SMTP credentials (if using email notifications)
- [ ] Configure Discord webhook URL
- [ ] Set appropriate `scrape_delay_seconds` (8-15 seconds recommended)
- [ ] Mount persistent volumes for `/app/data` and `/app/cache`
- [ ] Configure reverse proxy (nginx) with HTTPS
- [ ] Set up log rotation for `scraper.log`
- [ ] Monitor disk usage (cache grows with images)

### Scaling

- **Current design**: Single-instance, SQLite-based
- **For high load**: Consider PostgreSQL + job queue (Redis/RabbitMQ)
- **Horizontal scaling**: Not supported (SQLite constraint)

### Backup

```bash
# Backup database
sqlite3 bluray-tracker.db ".backup bluray-tracker-backup.db"

# Or copy file
cp bluray-tracker.db bluray-tracker-$(date +%Y%m%d).db
```

---

## Extending the Application

### Future Enhancements (Stub Provided)

#### IMDb Integration
A stub class exists for enriching products with IMDb data:

```cpp
class ImdbIntegrator {
public:
    void enrich(Product& product) const;
    // TODO: Implement via OMDb API or similar
};
```

### Suggested Features

1. **Advanced filters** - Filter wishlist by format (4K only), price range
3. **Multiple notification channels** - Telegram, Slack, etc.
4. **User authentication** - Multi-user support
5. **Mobile app** - Native iOS/Android or PWA

---

## Troubleshooting

### Build Errors

**Error**: `fatal error: gumbo.h: No such file or directory`
**Solution**: Install libgumbo-dev: `sudo apt-get install libgumbo-dev`

**Error**: `undefined reference to curl_easy_init`
**Solution**: Install libcurl4-openssl-dev

### Runtime Errors

**Error**: `Failed to open database`
**Solution**: Ensure write permissions on database file and directory

**Error**: `Failed to fetch page: status 0`
**Solution**: Check network connectivity, DNS resolution

### Performance Issues

- **Slow scraping**: Normal. Respect rate limits with appropriate delays.
- **High memory usage**: Check for memory leaks in scraper HTML parsing
- **Database locks**: Reduce concurrent access or switch to WAL mode

---

## Contributing Guidelines

### Code Style

- Follow existing conventions (see Code Conventions section)
- Use clang-format with project settings (if available)
- Write self-documenting code with clear variable names
- Add comments for complex algorithms only

### Pull Request Process

1. Create feature branch: `git checkout -b feature/my-feature`
2. Implement changes with tests
3. Update CLAUDE.md if adding new features
4. Commit with conventional commits format:
   - `feat: add new scraper for site.nl`
   - `fix: handle edge case in price parsing`
   - `docs: update API reference`
5. Push and create pull request

### Testing

- Manual testing required (no automated tests currently)
- Test with real product URLs from Amazon.nl and Bol.com
- Verify notifications are sent correctly

---

## License & Legal

This project is for **personal use only**. Web scraping may violate terms of service of target websites. Use responsibly:

- Respect robots.txt
- Use appropriate delays between requests
- Do not overload servers
- Consider using official APIs when available

---

## Support & Contact

For issues, questions, or contributions:
- GitHub Issues: [metalglove/bluray-tracker](https://github.com/metalglove/bluray-tracker)
- Ensure you're on the latest commit when reporting issues

---

## Appendix: Quick Reference

### Command Summary
```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)

# Run web server
./build/bluray-tracker --run --port 8080

# Run scraper once
./build/bluray-tracker --scrape

# Docker
docker-compose up -d
docker-compose logs -f bluray-tracker
docker-compose exec bluray-tracker /bin/bash

# Database inspection
sqlite3 data/bluray-tracker.db
sqlite> SELECT * FROM wishlist;
sqlite> SELECT key, value FROM config;
```

### File Locations (Docker)
- Database: `/app/data/bluray-tracker.db`
- Cache: `/app/cache/*.jpg`
- Logs: `/app/data/bluray-tracker.log`, `/app/data/scraper.log`
- Binary: `/app/bluray-tracker`

---

**Last Updated**: 2026-01-19
**Version**: 2.1.0 (Modern UI with Price History & WebSocket support)
**Maintainer**: metalglove
