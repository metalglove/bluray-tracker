# 🎬 Blu-ray Tracker

A modern C++17 application for tracking Blu-ray and UHD 4K movie prices on Dutch e-commerce sites (Amazon.nl and Bol.com).

## ✨ Features

### 🎨 Modern Web UI
- **Sleek Single-Page Application** - Smooth, responsive interface with no page reloads
- **Dark/Light Mode** - Toggle between themes with preference persistence
- **Real-Time Updates** - WebSocket-powered live updates across all clients
- **Beautiful Design** - Purple/indigo gradients, smooth animations, professional typography
- **Mobile-Friendly** - Fully responsive design for phones, tablets, and desktops
- **Interactive Dashboard** - Animated statistics cards and quick actions
- **Modal Dialogs** - Elegant forms for adding and editing items
- **Toast Notifications** - Real-time feedback for all user actions
- **Debounced Search** - Instant, responsive search for wishlist and collection items

### 🔍 Smart Tracking
- **Automatic Scraping** - Scheduled price and availability monitoring
- **Price Alerts** - Get notified when items drop below your threshold
- **Stock Monitoring** - Know when out-of-stock items are available again
- **Multiple Notifications** - Discord webhooks and SMTP email support
- **Image Caching** - Product images cached locally with SHA256 hashing
- **Pagination** - Efficient browsing of large collections
- **Source Filtering** - Filter wishlist/collection by Amazon.nl or Bol.com
- **Smart Scraper** - Bol.com variant detection (4K vs standard) and rate limiting
- **Title Locking** - Prevent scraper from overwriting your custom titles
- **Price History** - Visual charts tracking price trends over time
- **Release Calendar** - Track upcoming Blu-ray releases from blu-ray.com
- **Custom Tags** - Organize your collection with colored tags
- **Ratings & Trailers** - Add TMDb/IMDb ratings and YouTube trailers
- **Edition Tracking** - Track special editions, steelbooks, and bonus features

### 🛠️ Technical Excellence
- **Modern C++17** - robust and efficient codebase using `fmt` for formatting
- **Security Hardening** - SQL injection prevention and comprehensive log sanitization
- **Clean Architecture** - Domain, infrastructure, application, and presentation layers
- **SQLite Database** - Lightweight, embedded storage with RAII wrappers
- **Docker Ready** - Easy deployment with Docker Compose
- **WebSocket Support** - Real-time bidirectional communication
- **Thread-Safe** - Mutex-protected shared resources

## Quick Start

### Using Docker (Recommended)

```bash
# Clone the repository
git clone https://github.com/metalglove/bluray-tracker.git
cd bluray-tracker

# Ensure data directories exist and are writable
mkdir -p data cache
chmod 777 data cache

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

1. **Navigate** to `http://localhost:8080`
2. **Choose your theme** - Toggle dark/light mode in the header
3. **Add items** to your wishlist with desired maximum price
4. **Monitor prices** - The scraper runs automatically (every 6 hours by default)
5. **Get notified** - Real-time updates when prices drop or items are back in stock
6. **Manage settings** - Configure Discord/email notifications via Settings page

#### UI Features
- 📊 **Dashboard** - View statistics, quick actions, and upcoming releases
- 🎬 **Release Calendar** - Browse upcoming Blu-ray releases with one-click wishlist adds
- ⭐ **Wishlist** - Manage price-tracked items with filters
- 📀 **Collection** - Browse your owned items
- ⚙️ **Settings** - Configure scraping and notifications
- 🌓 **Theme Toggle** - Switch between dark and light modes
- 🔴 **Live Status** - Connection indicator shows real-time sync status

**Note**: On first startup, the release calendar automatically fetches upcoming releases from blu-ray.com. This ensures the dashboard looks great immediately!

### Configuration

All configuration is stored in the SQLite database and can be managed via the **Settings page** in the web UI.

**Key Settings:**
- **scrape_delay_seconds**: Delay between scraping requests (default: 8)
- **discord_webhook_url**: Discord webhook for notifications
- **smtp_server**, **smtp_port**, **smtp_user**, **smtp_pass**: Email configuration
- **smtp_from**, **smtp_to**: Email addresses for notifications
- **web_port**: Web server port (default: 8080)
- **cache_directory**: Location for cached images (default: ./cache)

**Via Web UI:**
1. Navigate to Settings page (⚙️ icon in sidebar)
2. Update desired values
3. Click "Save Settings"
4. Changes take effect immediately

**Via SQL:**
```sql
sqlite3 bluray-tracker.db "UPDATE config SET value='15' WHERE key='scrape_delay_seconds';"
```

### Manual Scraping

Trigger scraping manually:

```bash
# Scrape wishlist prices (checks Amazon.nl & Bol.com for price/stock changes)
./bluray-tracker --scrape --db bluray-tracker.db

# Scrape release calendar (fetches upcoming releases from blu-ray.com)
./bluray-tracker --scrape-calendar --db bluray-tracker.db

# Via API (wishlist only)
curl -X POST http://localhost:8080/api/scrape
```

**Automatic Schedules** (via cron in Docker):
- **Wishlist prices**: Every 6 hours (catches price drops and stock changes)
- **Release calendar**: Once daily at 3 AM (new releases update slowly)

**First Startup**: The release calendar automatically fetches initial data when the web server starts with an empty calendar. No manual scraping required!

## API Endpoints

### REST API

#### Wishlist
- `GET /api/wishlist?page=1&size=20` - List items (paginated)
- `POST /api/wishlist` - Add item
- `PUT /api/wishlist/{id}` - Update item
- `DELETE /api/wishlist/{id}` - Remove item

#### Collection
- `GET /api/collection?page=1&size=20` - List items (paginated)
- `POST /api/collection` - Add item
- `DELETE /api/collection/{id}` - Remove item

#### Dashboard & Actions
- `GET /api/stats` - Get dashboard statistics
- `POST /api/scrape` - Trigger manual scrape

#### Settings
- `GET /api/settings` - Get configuration
- `PUT /api/settings` - Update configuration

### WebSocket API

**Endpoint:** `WS /ws`

**Connection:** Automatic connection from web UI with auto-reconnect

**Events:**
- `wishlist_added` - New wishlist item created
- `wishlist_updated` - Wishlist item modified
- `wishlist_deleted` - Wishlist item removed
- `collection_added` - New collection item created
- `collection_deleted` - Collection item removed
- `scrape_completed` - Scraping finished with item count

**Example:**
```javascript
const ws = new WebSocket('ws://localhost:8080/ws');
ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  console.log('Update:', data.type);
};
```

All CRUD operations automatically broadcast updates to all connected clients for real-time synchronization.

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

- **C++17** with modern libraries
- **fmt** - Safe and fast formatting library (replacing std::format)
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

Two separate schedules for optimal performance. Customize in `docker-crontab`:

```bash
# Wishlist price scraping - every 6 hours
0 */6 * * * /app/bluray-tracker --scrape --db /app/data/bluray-tracker.db

# Release calendar scraping - once daily at 3 AM
0 3 * * * /app/bluray-tracker --scrape-calendar --db /app/data/bluray-tracker.db
```

**Why different schedules?**
- Prices change frequently → check every 6 hours to catch deals
- New releases are announced slowly → once daily is sufficient

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

## Recent Updates (v1.1)

### ✨ New Features
- **🗓️ Release Calendar** - Browse upcoming Blu-ray releases with one-click wishlist adds
- **⭐ Ratings Display** - Color-coded TMDb/IMDb ratings on all items
- **🎬 Movie Trailers** - YouTube trailer player with modal support
- **🏷️ Custom Tags** - Organize items with custom colored tags
- **🎁 Bonus Features** - Track edition type, slipcover, digital copy, and more

See [FEATURES.md](FEATURES.md) for the complete feature list and [ROADMAP.md](ROADMAP.md) for upcoming enhancements.

---

**Built with ❤️ and modern C++17**
