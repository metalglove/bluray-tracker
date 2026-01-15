# 🎬 Blu-ray Tracker

A modern C++20 application for tracking Blu-ray and UHD 4K movie prices on Dutch e-commerce sites (Amazon.nl and Bol.com).

## Features

- 📊 **Web Dashboard** - Clean, responsive interface for managing your collection
- 🔍 **Smart Scraping** - Automatic price and availability tracking
- 💰 **Price Alerts** - Get notified when items drop below your threshold
- 📦 **Stock Monitoring** - Know when out-of-stock items are available again
- 🔔 **Multiple Notifications** - Discord webhooks and SMTP email support
- 🖼️ **Image Caching** - Product images cached locally for fast loading
- 🗄️ **SQLite Database** - Lightweight, embedded storage
- 🐳 **Docker Ready** - Easy deployment with Docker Compose
- ⏰ **Scheduled Scraping** - Configurable cron-based automation

## Quick Start

### Using Docker (Recommended)

```bash
# Clone the repository
git clone https://github.com/metalglove/bluray-tracker.git
cd bluray-tracker

# Start with docker-compose
docker-compose up -d

# Access the web interface
open http://localhost:8080
```

### Manual Build

#### Prerequisites

Ubuntu/Debian:
```bash
sudo apt-get install -y \
    build-essential cmake git \
    libcurl4-openssl-dev \
    libgumbo-dev \
    libsqlite3-dev \
    libssl-dev
```

#### Build & Run

```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run web server
./build/bluray-tracker --run --port 8080
```

## Usage

### Web Interface

1. Navigate to `http://localhost:8080`
2. Add items to your wishlist with desired maximum price
3. The scraper runs automatically (every 6 hours by default)
4. Get notifications when prices drop or items are back in stock

### Configuration

All configuration is stored in the SQLite database. Key settings:

- **scrape_delay_seconds**: Delay between scraping requests (default: 8)
- **discord_webhook_url**: Discord webhook for notifications
- **smtp_server**, **smtp_user**, **smtp_pass**: Email notification settings
- **web_port**: Web server port (default: 8080)

Update via SQL:
```sql
sqlite3 bluray-tracker.db "UPDATE config SET value='15' WHERE key='scrape_delay_seconds';"
```

### Manual Scraping

Trigger scraping manually:

```bash
# Via CLI
./bluray-tracker --scrape --db bluray-tracker.db

# Via API
curl -X POST http://localhost:8080/api/scrape
```

## API Endpoints

### Wishlist

- `GET /api/wishlist?page=1&size=20` - List items
- `POST /api/wishlist` - Add item
- `PUT /api/wishlist/{id}` - Update item
- `DELETE /api/wishlist/{id}` - Remove item

### Collection

- `GET /api/collection?page=1&size=20` - List items
- `POST /api/collection` - Add item
- `DELETE /api/collection/{id}` - Remove item

### Actions

- `POST /api/scrape` - Trigger manual scrape

## Architecture

Built with clean architecture principles:

- **Domain Layer**: Pure business logic (models, change detection)
- **Infrastructure Layer**: Technical implementations (database, network, caching)
- **Application Layer**: Business orchestration (scraping, notifications)
- **Presentation Layer**: Web interface (Crow framework)

**Design Patterns Used**:
- Observer (change notifications)
- Factory (scraper creation)
- Repository (data access)
- Strategy (notification methods)
- Singleton (shared services)

## Technology Stack

- **C++20** with modern features (std::format, concepts, ranges)
- **CMake 3.22+** with FetchContent
- **Crow** - Fast C++ web framework
- **libcurl** - HTTP client
- **gumbo-parser** - HTML5 parser
- **SQLite3** - Embedded database
- **nlohmann/json** - Modern JSON library

## Development

See [CLAUDE.md](CLAUDE.md) for comprehensive development documentation including:
- Project structure
- Build instructions
- Code conventions
- API reference
- Troubleshooting guide

## Docker Deployment

### Environment Variables

Set in `docker-compose.yml`:

```yaml
environment:
  - TZ=Europe/Amsterdam
```

### Volumes

- `/app/data` - SQLite database and logs
- `/app/cache` - Cached product images

### Cron Schedule

Default: Every 6 hours. Customize in `docker-crontab`:

```bash
0 */6 * * * /app/bluray-tracker --scrape ...
```

## Legal Notice

This project is for **personal use only**. Web scraping may violate terms of service. Use responsibly:

- Respect rate limits (8+ second delays recommended)
- Don't overload servers
- Consider using official APIs when available
- Check robots.txt policies

## Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Follow existing code style
4. Test thoroughly
5. Submit a pull request

## License

This project is provided as-is for educational and personal use.

## Acknowledgments

- [Crow](https://github.com/CrowCpp/Crow) - C++ web framework
- [nlohmann/json](https://github.com/nlohmann/json) - Modern JSON library
- [gumbo-parser](https://github.com/google/gumbo-parser) - HTML5 parser

---

**Built with ❤️ and modern C++20**
