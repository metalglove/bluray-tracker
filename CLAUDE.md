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
