# CLAUDE.md - AI Assistant Guide for Blu-ray Tracker

## Project Overview

**Blu-ray Tracker** is a modern C++20 application for tracking Blu-ray/UHD 4K movie prices and availability on Dutch e-commerce sites (Amazon.nl and Bol.com). The application periodically scrapes product information, detects meaningful changes (price drops, stock updates), and sends notifications via Discord webhooks or email.

### Key Features
- Web-based dashboard (Crow framework)
- Periodic scraping with configurable delays
- Wishlist management with price thresholds
- Personal collection tracking
- Discord webhook and SMTP email notifications
- Image caching with SHA256-based filenames
- SQLite database for persistence
- Docker deployment with cron scheduling

### Technology Stack
- **Language**: C++20 (std::format, concepts, ranges, std::jthread, designated initializers)
- **Build System**: CMake 3.22+ with FetchContent
- **Web Framework**: Crow (header-only)
- **HTTP Client**: libcurl (system package)
- **HTML Parser**: gumbo-parser (system package)
- **Database**: SQLite3 (system package)
- **JSON**: nlohmann/json (header-only via FetchContent)

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
│   │   ├── logger.hpp/cpp         # Thread-safe logging (std::format)
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
   - Web interface using Crow framework
   - REST API + embedded HTML dashboard

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

#### Scraper Mode (one-time execution)
```bash
./bluray-tracker --scrape --db ./bluray-tracker.db
```

Use this mode in cron jobs for periodic scraping.

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

### Modern C++20 Features

#### Use std::format for string formatting
```cpp
std::string message = std::format("Price: €{:.2f}", price);
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

- `GET /api/wishlist?page=1&size=20` - List wishlist (paginated)
- `POST /api/wishlist` - Add item
  ```json
  {
    "url": "https://...",
    "title": "Movie Title",
    "desired_max_price": 19.99,
    "notify_on_price_drop": true,
    "notify_on_stock": true
  }
  ```
- `PUT /api/wishlist/{id}` - Update item
- `DELETE /api/wishlist/{id}` - Remove item

#### Collection

- `GET /api/collection?page=1&size=20` - List collection (paginated)
- `POST /api/collection` - Add item
  ```json
  {
    "url": "https://...",
    "title": "Movie Title",
    "purchase_price": 15.99,
    "is_uhd_4k": true,
    "notes": "Purchased on sale"
  }
  ```
- `DELETE /api/collection/{id}` - Remove item

#### Actions

- `POST /api/scrape` - Trigger manual scrape
  ```json
  { "success": true, "processed": 15 }
  ```

#### Static

- `GET /cache/{filename}` - Serve cached product image

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

1. **Price charts** - Visualize price history over time
2. **Advanced filters** - Filter wishlist by format (4K only), price range
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

**Last Updated**: 2026-01-15
**Version**: 1.0.0
**Maintainer**: metalglove
