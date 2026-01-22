# Blu-ray Tracker - Feature Documentation & Planning

**Version**: 1.1
**Date**: 2026-01-22
**Status**: Living Document
**Last Verification**: 2026-01-22 - Codebase audit complete, documentation synchronized

---

## 🎉 Recent Completions

Since the last documentation update, the following "Tier 1 Quick Wins" have been fully implemented:

1. ✅ **TMDb/IMDb Rating Display** - Color-coded rating badges on all items (manual entry)
2. ✅ **Movie Trailers Integration** - YouTube trailer player with modal support
3. ✅ **Custom Tags & Labels** - Full CRUD API with colored tag pills and management modal
4. ✅ **Bonus Features Tracking** - Edition type, slipcover, digital copy, and bonus features fields
5. ✅ **Release Calendar** - Blu-ray.com scraper with calendar view and auto-population on first run

**Next Priority**: Automatic TMDb API enrichment to populate metadata fields automatically

---

## Table of Contents

1. [Current Implemented Features](#current-implemented-features)
2. [Existing Roadmap Features](#existing-roadmap-features)
3. [New Feature Ideas](#new-feature-ideas)
4. [Feature Prioritization Matrix](#feature-prioritization-matrix)
5. [Competitive Analysis](#competitive-analysis)

---

## Current Implemented Features

### Core Infrastructure

| Feature | Description | Status |
|---------|-------------|--------|
| **Clean Architecture** | 4-layer architecture (domain, infrastructure, application, presentation) | ✅ Complete |
| **SQLite Database** | Persistent storage with schema migrations | ✅ Complete |
| **Thread-Safe Singletons** | Logger, DatabaseManager, ConfigManager | ✅ Complete |
| **RAII Resource Management** | Smart pointers, transaction guards | ✅ Complete |

### Web Frontend (Modern SPA)

| Feature | Description | Status |
|---------|-------------|--------|
| **Responsive Design** | Mobile-friendly with collapsible sidebar | ✅ Complete |
| **Dark/Light Theme Toggle** | Persistent theme preference in localStorage | ✅ Complete |
| **Real-Time WebSocket Updates** | Live notifications for all CRUD operations | ✅ Complete |
| **Toast Notifications** | User feedback for actions | ✅ Complete |
| **Modal Dialogs** | Add/edit items with form validation | ✅ Complete |
| **Pagination** | Server-side pagination for lists | ✅ Complete |
| **Debounced Search** | Responsive search across collections | ✅ Complete |
| **Dashboard Statistics** | Animated stats cards with counts | ✅ Complete |

### Wishlist Management

| Feature | Description | Status |
|---------|-------------|--------|
| **Add/Edit/Delete Items** | Full CRUD operations | ✅ Complete |
| **Price Threshold Alerts** | Notification when price drops below target | ✅ Complete |
| **Stock Monitoring** | Track in-stock/out-of-stock status | ✅ Complete |
| **Title Locking** | Prevent scraper from overwriting custom titles | ✅ Complete |
| **Format Detection** | Automatic UHD 4K vs Blu-ray identification | ✅ Complete |
| **Source Identification** | Amazon.nl / Bol.com badges | ✅ Complete |

### Collection Management

| Feature | Description | Status |
|---------|-------------|--------|
| **Add/Edit/Delete Items** | Full CRUD operations | ✅ Complete |
| **Purchase Price Tracking** | Record what you paid | ✅ Complete |
| **Notes Field** | Personal notes per item | ✅ Complete |
| **Format Tracking** | UHD 4K / Blu-ray distinction | ✅ Complete |

### Price History

| Feature | Description | Status |
|---------|-------------|--------|
| **Automatic Recording** | Price logged on every scrape | ✅ Complete |
| **Chart.js Visualization** | Interactive line charts with tooltips | ✅ Complete |
| **Target Price Line** | Visual reference on charts | ✅ Complete |
| **History Modal** | View price history per item | ✅ Complete |
| **Configurable Retention** | 180-day default, 365-day max | ✅ Complete |

### Release Calendar

| Feature | Description | Status |
|---------|-------------|--------|
| **Blu-ray.com Scraper** | Fetch upcoming Dutch releases | ✅ Complete |
| **Calendar View** | Dedicated release calendar page | ✅ Complete |
| **Auto-Population** | First-run automatic data fetch | ✅ Complete |
| **Add to Wishlist Integration** | Quick-add from calendar items | ✅ Complete |
| **Pre-order Tracking** | Flag for pre-order items | ✅ Complete |

### Scrapers

| Scraper | Capabilities | Status |
|---------|--------------|--------|
| **Amazon.nl** | Price, stock, title, image, 4K detection | ✅ Complete |
| **Bol.com** | Price, stock, title, image, variant detection | ✅ Complete |
| **Blu-ray.com** | Release calendar data | ✅ Complete |

### Notifications

| Channel | Features | Status |
|---------|----------|--------|
| **Discord Webhook** | Price drop, back-in-stock alerts | ✅ Complete |
| **Email (SMTP)** | Configurable SMTP settings | ✅ Complete |

### Image Caching

| Feature | Description | Status |
|---------|-------------|--------|
| **SHA256-based Filenames** | Deduplication of images | ✅ Complete |
| **Local Cache Serving** | `/cache/{filename}` route | ✅ Complete |
| **Long-term Caching Headers** | 1-year browser cache | ✅ Complete |

### Security & Performance

| Feature | Description | Status |
|---------|-------------|--------|
| **SQL Injection Prevention** | Parameterized queries | ✅ Complete |
| **XSS Prevention** | HTML escaping | ✅ Complete |
| **Log Sanitization** | No sensitive data in logs | ✅ Complete |
| **Rate Limiting** | Configurable scrape delays | ✅ Complete |

---

## Existing Roadmap Features

### Phase 1 (Completed)
- ✅ Price History Visualization
- ✅ Title Locking
- ✅ Collection Editing
- ✅ Scraper Rate Limiting
- ✅ Debounced Search
- ✅ Security Hardening
- ✅ Release Calendar (Blu-ray.com integration)
- ✅ TMDb/IMDb Rating Display (Manual entry via UI)
- ✅ Movie Trailers Integration (YouTube keys)
- ✅ Custom Tags & Labels
- ✅ Bonus Features Tracking (Edition type, slipcover, digital copy)

### Phase 2: Enhanced Discovery & Alerts (Next Priority)

| Feature | Effort | Value | Status |
|---------|--------|-------|--------|
| **Automatic TMDb Metadata Enrichment** | 3 weeks | Very High | **Fields Ready, API Integration Needed** |
| **IMDb Discovery/Recommendations** | 3-4 weeks | High | Depends on above |
| **Multi-Site Expansion** | 2-3 weeks/site | High | Extensible |
| **Custom Alert Rules** | 3 weeks | High | Infrastructure Ready |

### Phase 3: Advanced UX & Infrastructure

| Feature | Effort | Value | Status |
|---------|--------|-------|--------|
| **Inventory Management (Barcode)** | 3 weeks | Medium-High | Planned |
| **Backup & Cloud Sync** | 4 weeks | High | Planned |
| **Progressive Web App (PWA)** | 4-5 weeks | Medium | Planned |

### Phase 4: Social & Polish

| Feature | Effort | Value | Status |
|---------|--------|-------|--------|
| **Social Sharing (X/Twitter)** | 2-3 weeks | Low-Medium | Planned |

---

## New Feature Ideas

Based on user suggestions, competitive analysis, and research, here are additional features to consider:

---

### 🎬 1. Movie Trailers Integration ✅ **IMPLEMENTED**

**Priority**: HIGH
**Effort**: Low-Medium (1-2 weeks)
**Dependencies**: TMDb API (same as metadata enrichment)
**Status**: ✅ Complete - Manual entry supported via UI

#### Description
Embed official movie trailers directly in the item detail views for wishlist and collection items.

#### Implementation Approach

**API Integration (TMDb)**
- Endpoint: `https://api.themoviedb.org/3/movie/{id}/videos?api_key={key}`
- Returns YouTube/Vimeo keys for trailers, clips, featurettes
- Filter by `type: "Trailer"` for official trailers

**Alternative: KinoCheck API**
- Free access for publishers
- Supports TMDb IDs directly: `GET https://api.kinocheck.com/trailers?tmdb_id=299534`
- Higher quality (4K), more comprehensive

**Frontend Implementation**
```javascript
// Embed YouTube trailer
const embedUrl = `https://www.youtube.com/embed/${videoKey}`;
// Or use YouTube's thumbnail for lazy loading
const thumbnail = `https://img.youtube.com/vi/${videoKey}/maxresdefault.jpg`;
```

**UI/UX**
- "Watch Trailer" button on each item card
- Modal with embedded YouTube player
- Thumbnail preview with play button overlay
- Optional: Auto-play on hover (muted)

**Database Schema**
```sql
ALTER TABLE wishlist ADD COLUMN trailer_key TEXT;
ALTER TABLE collection ADD COLUMN trailer_key TEXT;
```

#### Success Metrics
- 80%+ of items have trailers available
- Trailer modal loads in <2 seconds

---

### 💰 2. Deals Page / Deal Finder

**Priority**: HIGH
**Effort**: Medium (3-4 weeks)
**Dependencies**: All scrapers

#### Description
A dedicated page showing the best current deals across all supported retailers, with price history context and filtering.

#### Implementation Approach

**Deal Detection Logic**
```cpp
struct Deal {
    WishlistItem item;
    float discount_percentage;
    float price_drop_amount;
    std::string reason;  // "40% off", "All-time low", "Below target"
};

// Criteria for "deal":
// 1. Price dropped >15% from last scrape
// 2. Below historical average
// 3. At or near all-time low (within 10%)
// 4. Below user's target price
```

**External Deal Sources**
- Scrape Amazon.nl "Deals" page: `https://www.amazon.nl/events/deals`
- Scrape Bol.com "Aanbiedingen" page
- Parse "Dagdeal" (daily deals) sections

**Reference: Keepa/CamelCamelCamel Model**
- Show price history sparkline next to each deal
- "All-time low" badge
- "Back to normal in X hours" countdown for lightning deals

**Frontend Page**
- `/deals` route
- Filters: Format (4K/BD), Source, Discount %, Category
- Sort: Biggest discount, Price low-to-high, Ending soon
- "Add to Wishlist" quick action
- Real-time updates via WebSocket

**API Endpoint**
```
GET /api/deals?min_discount=20&format=4k&source=amazon.nl
```

#### Success Metrics
- 10+ deals displayed at any given time
- 30% of users visit deals page weekly

---

### 🔊 3. Audio & Subtitle Tracks Information

**Priority**: MEDIUM-HIGH
**Effort**: Medium (2-3 weeks)
**Dependencies**: Additional scraping or external database

#### Description
Display available audio tracks (Dolby Atmos, DTS:X, Dutch dubs) and subtitle languages for each Blu-ray.

#### Implementation Approach

**Data Sources**

1. **Blu-ray.com Database** (Preferred)
   - Most comprehensive technical specs
   - Forum thread confirms no official API, but scraping possible
   - URL pattern: `https://www.blu-ray.com/movies/{title}/######/`
   - Parse: Audio section, Subtitles section

2. **TMDb API** (Limited)
   - Only provides spoken languages, not disc-specific audio formats

3. **User-Contributed** (Fallback)
   - Allow manual entry of audio/subtitle info
   - Community-driven database

**Database Schema**
```sql
CREATE TABLE disc_specs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    wishlist_id INTEGER,
    collection_id INTEGER,
    audio_tracks TEXT,      -- JSON: [{"language": "English", "format": "Dolby Atmos", "channels": "7.1"}]
    subtitle_tracks TEXT,   -- JSON: ["English", "Dutch", "French"]
    video_resolution TEXT,  -- "4K UHD", "1080p"
    hdr_format TEXT,        -- "Dolby Vision", "HDR10+", "HDR10"
    region_code TEXT,       -- "A", "B", "C", "Free"
    runtime_minutes INTEGER,
    bonus_features TEXT,    -- JSON array
    FOREIGN KEY (wishlist_id) REFERENCES wishlist(id),
    FOREIGN KEY (collection_id) REFERENCES collection(id)
);
```

**Frontend Display**
- Audio badges: "🔊 Dolby Atmos" "🔊 DTS:X" "🎧 7.1"
- Subtitle flags: 🇳🇱 🇬🇧 🇫🇷 🇩🇪
- Expandable section in item detail modal
- Filter by: "Has Dutch subtitles", "Has Dolby Atmos"

#### Success Metrics
- 50%+ of items have audio/subtitle data
- Users can filter by audio format

---

### 🎁 4. Bonus Features / Special Edition Tracking ✅ **IMPLEMENTED**

**Priority**: MEDIUM
**Effort**: Low-Medium (1-2 weeks)
**Dependencies**: Scraper enhancement or manual entry
**Status**: ✅ Complete - Edition type, slipcover, digital copy, bonus features tracking available

#### Description
Track whether a Blu-ray includes bonus content like behind-the-scenes, deleted scenes, commentary tracks, steelbook packaging, etc.

#### Implementation Approach

**Data Sources**
- Scrape from product descriptions (Amazon/Bol)
- Blu-ray.com "Special Features" section
- User input

**Database Schema**
```sql
ALTER TABLE wishlist ADD COLUMN has_bonus_features INTEGER DEFAULT 0;
ALTER TABLE wishlist ADD COLUMN bonus_features_list TEXT;  -- JSON array
ALTER TABLE wishlist ADD COLUMN edition_type TEXT;  -- "Standard", "Steelbook", "Collector's", "Ultimate"
```

**Bonus Feature Categories**
```json
[
  "Audio Commentary",
  "Deleted Scenes",
  "Behind the Scenes",
  "Making Of",
  "Interviews",
  "Theatrical Trailer",
  "Digital Copy",
  "Art Cards",
  "Booklet",
  "Slipcover"
]
```

**Frontend**
- Badges: "📀 Bonus Features" "🎭 Steelbook"
- Filter: "Has Commentary", "Collector's Edition"
- Comparison: Show which edition has more features

#### Success Metrics
- 40%+ of items have bonus info
- Edition type tracked for all items

---

### ⭐ 5. IMDb/TMDb Rating Display ✅ **IMPLEMENTED**

**Priority**: HIGH (Already in Roadmap - this expands it)
**Effort**: Low (part of metadata enrichment)
**Dependencies**: TMDb API integration
**Status**: ✅ Complete - Manual entry supported, automatic enrichment pending

#### Description
Prominently display IMDb/TMDb ratings on all item cards with visual indicators.

#### Implementation Approach

**Display Formats**
```
⭐ 8.8/10 (IMDb)
🍅 94% (Rotten Tomatoes - if available)
👤 4.2/5 (User rating - future feature)
```

**Visual Design**
- Color-coded badges:
  - Green: 8.0+ (Excellent)
  - Yellow: 6.0-7.9 (Good)
  - Orange: 4.0-5.9 (Average)
  - Red: <4.0 (Poor)
- Circular progress indicator for rating

**Sorting & Filtering**
- Sort by: Rating (high to low)
- Filter: "Highly Rated (8+)", "Hidden Gems (7+ with <10K votes)"

**API Data**
```json
{
  "vote_average": 8.8,
  "vote_count": 2500000,
  "popularity": 85.5
}
```

#### Success Metrics
- Ratings displayed for 90%+ of enriched items
- Users use rating sort/filter weekly

---

### 🎨 6. UI Overhaul: Card-Based Collection View

**Priority**: HIGH
**Effort**: Medium-High (3-4 weeks)
**Dependencies**: None (frontend only)

#### Description
Transform the current table-based view into a modern, Netflix-style card grid with hover effects, posters as primary visual, and quick actions.

#### Design Inspiration
- Netflix browse interface
- Plex media server
- Letterboxd film collection
- CLZ Movies app

#### Implementation Approach

**Card Layout**
```css
/* Responsive grid */
.collection-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(180px, 1fr));
  gap: 24px;
}

/* Movie card */
.movie-card {
  aspect-ratio: 2/3;  /* Standard poster ratio */
  border-radius: 12px;
  overflow: hidden;
  transition: transform 0.3s, box-shadow 0.3s;
}

.movie-card:hover {
  transform: scale(1.05);
  box-shadow: 0 20px 40px rgba(0,0,0,0.4);
}
```

**Card Content**
- **Front**: High-quality poster image (full bleed)
- **Hover Overlay**:
  - Title
  - Rating badge
  - Price (for wishlist)
  - Quick actions: ❤️ Add to Collection, 🛒 Buy, 🎬 Watch Trailer
- **Badges** (top corner):
  - "4K" / "BD" format
  - "IN STOCK" / "OUT OF STOCK"
  - Price drop indicator (↓ -20%)

**View Toggle**
- Allow users to switch between:
  - 📊 Table View (current, for detailed info)
  - 🖼️ Card Grid View (visual browsing)
  - 📋 List View (compact, text-focused)
- Persist preference in localStorage

**Sorting & Filtering Bar**
- Sticky header with:
  - Search input
  - Sort dropdown (Title, Price, Rating, Date Added)
  - Format filter chips (All, 4K, Blu-ray)
  - Source filter chips (All, Amazon, Bol)

**Skeleton Loading**
- Show animated placeholder cards while loading
- Smooth fade-in for images

**Keyboard Navigation**
- Arrow keys to navigate between cards
- Enter to open detail modal
- Escape to close modals

#### Example Card Component (HTML)
```html
<div class="movie-card" data-id="123">
  <img src="/cache/poster.jpg" alt="Inception" loading="lazy">
  <div class="card-badges">
    <span class="badge badge-4k">4K</span>
    <span class="badge badge-stock">IN STOCK</span>
  </div>
  <div class="card-overlay">
    <h3 class="card-title">Inception</h3>
    <div class="card-rating">⭐ 8.8</div>
    <div class="card-price">€19.99</div>
    <div class="card-actions">
      <button class="btn-trailer" title="Watch Trailer">🎬</button>
      <button class="btn-edit" title="Edit">✏️</button>
      <button class="btn-delete" title="Delete">🗑️</button>
    </div>
  </div>
</div>
```

#### Success Metrics
- 70% of users prefer card view over table
- Page load time <2s for 100 items
- Accessibility: Full keyboard navigation

---

### 📺 7. Blu-ray Player Tracking (Tweakers Integration)

**Priority**: MEDIUM
**Effort**: Medium-High (3-4 weeks)
**Dependencies**: New scraper for Tweakers.net

#### Description
Track Blu-ray player prices and availability from Dutch retailers, with price history and comparison features.

#### Implementation Approach

**New Section: Hardware**
- Separate from movie collection
- Categories: "4K Blu-ray Players", "Standard Blu-ray Players", "Multi-Region Players"

**Data Sources**

1. **Tweakers.net Pricewatch**
   - URL: `https://tweakers.net/pricewatch/`
   - Category: "Blu-ray spelers"
   - Features: Multi-retailer price comparison, price history

2. **Direct Retailer Scraping**
   - Coolblue.nl
   - MediaMarkt.nl
   - Amazon.nl

**Database Schema**
```sql
CREATE TABLE hardware (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    brand TEXT,
    model TEXT,
    category TEXT,  -- "4k_player", "bluray_player", "region_free"
    url TEXT NOT NULL UNIQUE,
    current_price REAL,
    lowest_price REAL,
    image_url TEXT,
    local_image_path TEXT,
    tweakers_url TEXT,
    specs TEXT,  -- JSON: {"hdmi": 2, "4k_upscaling": true, "dolby_vision": true}
    in_stock INTEGER DEFAULT 0,
    created_at TEXT,
    last_checked TEXT
);

CREATE TABLE hardware_price_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    hardware_id INTEGER,
    price REAL,
    source TEXT,
    recorded_at TEXT,
    FOREIGN KEY (hardware_id) REFERENCES hardware(id)
);
```

**Key Hardware Specs to Track**
- 4K UHD support
- HDR formats (Dolby Vision, HDR10, HDR10+, HLG)
- Audio passthrough (Dolby Atmos, DTS:X)
- Region code (A, B, C, Region-free)
- HDMI ports count
- Wi-Fi / Ethernet
- Streaming apps support
- Brand/Model

**Frontend Page: `/hardware`**
- Card grid of players with specs badges
- Price comparison across retailers
- Price history charts
- "Best Value" recommendations
- Filter by: Brand, Price range, Features (Dolby Vision, etc.)

**Popular Models to Track**
Based on 2025 recommendations:
- Panasonic DP-UB820 (Best Overall)
- Sony UBP-X700M (Best Value)
- Panasonic DP-UB9000 (Premium)
- Magnetar UDP900 (Region-free option)

#### Success Metrics
- 20+ hardware items tracked
- Price alerts work for hardware
- 10% of users track at least 1 player

---

### 🔍 8. Advanced Search & Filtering

**Priority**: MEDIUM-HIGH
**Effort**: Medium (2-3 weeks)
**Dependencies**: Enhanced database queries

#### Description
Powerful search with multiple criteria, saved searches, and smart suggestions.

#### Features

**Multi-Criteria Search**
```
title:"dark knight" price:<20 format:4k in_stock:yes rating:>8
```

**Search Operators**
| Operator | Example | Description |
|----------|---------|-------------|
| `title:` | `title:"inception"` | Search title |
| `director:` | `director:"nolan"` | Search director |
| `actor:` | `actor:"dicaprio"` | Search cast |
| `year:` | `year:2010` or `year:2010-2020` | Release year |
| `price:` | `price:<20` or `price:10-30` | Price range |
| `rating:` | `rating:>8` | Minimum rating |
| `format:` | `format:4k` or `format:bd` | Media format |
| `genre:` | `genre:action` | Genre filter |
| `source:` | `source:amazon` | Retailer |
| `in_stock:` | `in_stock:yes` | Availability |
| `has:` | `has:atmos` or `has:trailer` | Feature check |

**Saved Searches**
```sql
CREATE TABLE saved_searches (
    id INTEGER PRIMARY KEY,
    name TEXT,
    query TEXT,
    notify_on_new INTEGER DEFAULT 0,
    created_at TEXT
);
```

**Smart Suggestions**
- Autocomplete for titles, directors, actors
- "Did you mean..." for typos
- Recent searches dropdown

**Frontend**
- Search bar with dropdown for advanced options
- Filter chips that can be clicked to remove
- "Save this search" button
- Results count with applied filters shown

---

### 📱 9. Mobile Companion App (PWA Enhancement)

**Priority**: MEDIUM
**Effort**: High (5-6 weeks)
**Dependencies**: PWA foundation

#### Description
Enhance PWA with mobile-specific features like barcode scanning, offline mode, and push notifications.

#### Mobile-Specific Features

**Camera Integration**
- Scan barcodes to add items
- Take photos of collection for inventory
- AR view (future): Point at shelf to see prices

**Offline Support**
- Browse collection without internet
- Queue additions for sync when online
- Cache recent price data

**Push Notifications**
- Price drop alerts
- Back-in-stock alerts
- New release reminders
- Weekly deals digest

**Mobile UI Optimizations**
- Bottom navigation bar
- Swipe gestures for actions
- Pull-to-refresh
- Haptic feedback

---

### 🌐 10. Multi-Language Support (i18n)

**Priority**: LOW-MEDIUM
**Effort**: Medium (3 weeks)
**Dependencies**: UI refactoring

#### Description
Support multiple interface languages, starting with Dutch and English.

#### Implementation

**Language Files**
```json
// en.json
{
  "wishlist": "Wishlist",
  "add_to_wishlist": "Add to Wishlist",
  "price_dropped": "Price dropped to €{price}!"
}

// nl.json
{
  "wishlist": "Verlanglijst",
  "add_to_wishlist": "Toevoegen aan verlanglijst",
  "price_dropped": "Prijs gedaald naar €{price}!"
}
```

**Browser Detection**
- Auto-detect from `Accept-Language` header
- Allow manual override in settings
- Persist preference

---

### 📊 11. Statistics & Analytics Dashboard

**Priority**: MEDIUM
**Effort**: Medium (2-3 weeks)
**Dependencies**: Price history data

#### Description
Comprehensive analytics about your collection, spending, and market trends.

#### Dashboard Widgets

**Collection Stats**
- Total items (by format)
- Total collection value (current prices)
- Total spent vs. current value
- Average price per item
- Most expensive item
- Best deal (biggest savings)

**Price Trends**
- Average 4K Blu-ray price over time
- Seasonal trends (Black Friday dips)
- Per-retailer price comparison

**Genre Distribution**
- Pie chart of collection by genre
- Most collected directors/actors

**Activity Timeline**
- Recent additions
- Recent purchases
- Price changes on wishlist

**Visualizations**
- Line charts (price history)
- Pie charts (genre distribution)
- Bar charts (monthly spending)
- Heatmap (purchase activity calendar)

---

### 🔔 12. Smart Notifications & Digest Emails

**Priority**: MEDIUM
**Effort**: Medium (2-3 weeks)
**Dependencies**: Email system exists

#### Description
Intelligent notification batching and weekly digest emails.

#### Notification Types

**Instant Alerts**
- Price drops below target
- Back in stock for wishlisted items
- Lightning deals on wishlist items

**Daily Digest** (configurable time)
- Summary of price changes
- New deals discovered
- Upcoming releases this week

**Weekly Newsletter**
- Best deals of the week
- New releases
- Collection statistics
- Recommendations based on collection

**Notification Preferences**
```sql
CREATE TABLE notification_preferences (
    id INTEGER PRIMARY KEY,
    type TEXT,  -- "instant", "daily_digest", "weekly"
    channel TEXT,  -- "email", "discord", "push"
    enabled INTEGER DEFAULT 1,
    time_of_day TEXT  -- For digest: "08:00"
);
```

---

### 🏷️ 13. Custom Tags & Labels ✅ **IMPLEMENTED**

**Priority**: MEDIUM
**Effort**: Low (1-2 weeks)
**Dependencies**: None
**Status**: ✅ Complete - Full CRUD API, tag management modal, colored tags display

#### Description
User-defined tags for organizing collections beyond built-in categories.

#### Implementation

**Database**
```sql
CREATE TABLE tags (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    color TEXT DEFAULT '#667eea'
);

CREATE TABLE item_tags (
    item_id INTEGER,
    item_type TEXT,  -- "wishlist" or "collection"
    tag_id INTEGER,
    PRIMARY KEY (item_id, item_type, tag_id)
);
```

**Example Tags**
- "Christmas Gifts"
- "To Watch"
- "Favorites"
- "Lending to John"
- "Replace with 4K"

**Frontend**
- Tag pills with colors
- Filter by tag
- Quick-add tags via dropdown

---

### 🔗 14. External Integrations

**Priority**: LOW-MEDIUM
**Effort**: Varies

#### Letterboxd Integration
- Import watchlist from Letterboxd
- Export collection to Letterboxd
- Display Letterboxd ratings

#### Trakt.tv Integration
- Sync watched status
- Import watchlist

#### Plex/Jellyfin Integration
- Match physical collection to digital library
- "Also available digitally" indicator

#### Home Assistant Integration
- MQTT notifications
- Sensors for collection stats

---

### 💾 15. Import/Export Functionality

**Priority**: MEDIUM
**Effort**: Low-Medium (2 weeks)
**Dependencies**: None

#### Description
Import collections from other apps and export data in various formats.

#### Import Sources
- CSV file upload
- CLZ Movies export
- My Movies by Blu-ray.com export
- iCollect export
- Manual bulk add (paste URLs)

#### Export Formats
- CSV (Excel-compatible)
- JSON (backup/migration)
- PDF (printable collection list)
- HTML (shareable static page)

**API Endpoints**
```
POST /api/import?format=csv
GET /api/export?format=csv&type=collection
```

---

### 🛡️ 16. User Accounts & Multi-User Support

**Priority**: LOW (significant architecture change)
**Effort**: High (6+ weeks)
**Dependencies**: Authentication system

#### Description
Support multiple users with separate collections on the same instance.

#### Features
- User registration/login
- Personal collections per user
- Shared family collection option
- Admin dashboard
- Permission levels (admin, user, viewer)

---

### 🎯 17. Price Prediction

**Priority**: LOW
**Effort**: High
**Dependencies**: ML infrastructure, extensive price history

#### Description
Machine learning-based price predictions based on historical data.

#### Features
- "Wait for Black Friday?" recommendation
- "Price likely to drop" indicator
- Best time to buy predictions
- Seasonal pattern analysis

---

## Feature Prioritization Matrix

### Scoring (Value 1-5, Effort 1-5, Score = Value/Effort)

| # | Feature | Value | Effort | Score | Status |
|---|---------|-------|--------|-------|--------|
| 5 | IMDb Rating Display | 5 | 1 | **5.00** | ✅ **DONE** |
| 1 | Movie Trailers | 4 | 2 | **2.00** | ✅ **DONE** |
| 4 | Bonus Features | 3 | 2 | **1.50** | ✅ **DONE** |
| 13 | Custom Tags | 3 | 2 | **1.50** | ✅ **DONE** |
| 2 | Deals Page | 5 | 3 | **1.67** | 📋 Planned |
| 6 | Card-Based UI | 5 | 3 | **1.67** | 📋 Planned |
| 3 | Audio/Subtitles | 4 | 3 | **1.33** | 📋 Planned |
| 8 | Advanced Search | 4 | 3 | **1.33** | 📋 Planned |
| 11 | Statistics Dashboard | 4 | 3 | **1.33** | 📋 Planned |
| 12 | Smart Notifications | 4 | 3 | **1.33** | |
| 7 | Blu-ray Players (Tweakers) | 3 | 4 | **0.75** | |
| 15 | Import/Export | 3 | 2 | **1.50** | ✅ |
| 10 | Multi-Language | 3 | 3 | **1.00** | |
| 9 | Mobile PWA | 4 | 5 | **0.80** | |
| 14 | External Integrations | 3 | 4 | **0.75** | |
| 16 | Multi-User | 3 | 5 | **0.60** | |
| 17 | Price Prediction | 3 | 5 | **0.60** | |

### Recommended Implementation Order

**Tier 1: Quick Wins (1-2 weeks each)** ✅ **COMPLETED**
1. ✅ ⭐ IMDb Rating Display (manual entry - automatic enrichment pending)
2. ✅ 🎬 Movie Trailers Integration
3. ✅ 🎁 Bonus Features Tracking
4. ✅ 🏷️ Custom Tags & Labels

**Tier 2: High Impact (3-4 weeks each)**
5. 🎨 Card-Based UI Overhaul
6. 💰 Deals Page / Deal Finder
7. 🔊 Audio & Subtitle Tracks
8. 🔍 Advanced Search

**Tier 3: Extended Features (4+ weeks)**
9. 📺 Blu-ray Player Tracking
10. 📊 Statistics Dashboard
11. 📱 Mobile PWA Enhancement
12. 🔔 Smart Notifications

---

## Competitive Analysis

### Feature Comparison with Market Leaders

| Feature | Blu-ray Tracker | CLZ Movies | My Movies | iCollect |
|---------|----------------|------------|-----------|----------|
| Price Tracking | ✅ | ❌ | ❌ | ✅ (values) |
| Stock Alerts | ✅ | ❌ | ❌ | ❌ |
| Barcode Scanning | 📋 Planned | ✅ | ✅ | ✅ |
| IMDb Integration | 📋 Planned | ✅ | ✅ | ✅ |
| Cloud Sync | 📋 Planned | ✅ | ✅ | ✅ |
| Release Calendar | ✅ | ❌ | ✅ | ❌ |
| Price History | ✅ | ❌ | ❌ | ❌ |
| Dutch Retailers | ✅ | ❌ | ❌ | ❌ |
| Self-Hosted | ✅ | ❌ | ❌ | ❌ |
| Open Source | ✅ | ❌ | ❌ | ❌ |
| Card UI | 📋 Planned | ✅ | ✅ | ✅ |
| Multi-Platform | Web | Mobile+Web | Mobile+Web | Mobile+Desktop |
| Subscription | Free | $14.99/yr | Free + IAP | $4.99 one-time |

### Unique Selling Points

**What Blu-ray Tracker does better:**
1. **Price tracking** - Real-time Dutch retailer prices
2. **Deal alerts** - Proactive notifications for price drops
3. **Self-hosted** - Privacy-focused, no subscription
4. **Dutch market focus** - Amazon.nl, Bol.com support
5. **Price history** - Historical price charts for informed buying

**Areas to improve:**
1. Mobile experience (PWA)
2. Barcode scanning
3. Visual collection browsing (card UI)
4. Metadata richness (IMDb integration)

---

## Conclusion

This document outlines **17 potential new features** beyond the existing roadmap, ranging from quick wins (trailer integration, rating display) to ambitious projects (multi-user support, price prediction).

**Recommended focus areas based on user suggestions:**
1. 🎬 **Trailers** - Easy win with TMDb API
2. 💰 **Deals Page** - High value for deal hunters
3. 🔊 **Audio/Subtitles** - Differentiator feature
4. ⭐ **IMDb Ratings** - Already planned, high priority
5. 🎨 **Card UI** - Modern look, user-requested
6. 📺 **Blu-ray Players** - Unique market feature

---

**Document Version**: 1.0
**Last Updated**: 2026-01-21
**Maintainer**: Claude (AI Assistant)

---

## References

### Sources
- [CLZ Movies](https://clz.com/movies) - Movie collection app
- [My Movies by Blu-ray.com](https://apps.apple.com/us/app/my-movies-by-blu-ray-com/id514571960) - Blu-ray.com official app
- [iCollect Movies](https://www.icollecteverything.com/movies/) - Collection tracker
- [Coollector](https://www.coollector.com/) - Free movie database
- [Keepa](https://keepa.com/) - Amazon price tracker
- [TMDb API](https://www.themoviedb.org/talk/5f674bc67707000036125ba8) - Movie trailer integration
- [KinoCheck API](https://api.kinocheck.com/) - Official movie trailers
- [FreeFrontend CSS Movie Cards](https://freefrontend.com/css-movie-cards/) - UI design inspiration
- [What Hi-Fi Best Blu-ray Players 2025](https://www.whathifi.com/best-buys/home-cinema/best-blu-ray-and-4k-blu-ray-players) - Hardware recommendations
