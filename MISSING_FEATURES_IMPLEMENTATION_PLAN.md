# Missing Features Implementation Plan

**Version**: 1.0
**Date**: 2026-01-22
**Branch**: claude/plan-missing-features-Li051
**Status**: Planning Phase

---

## Executive Summary

This document identifies all features from ROADMAP.md and FEATURES.md that are **NOT yet implemented** and provides a comprehensive, prioritized implementation plan.

### Current Status Audit

**✅ COMPLETED (Phase 1)**
- Price History Visualization
- Release Calendar (Blu-ray.com integration)
- TMDb/IMDb Rating Display
- Movie Trailers Integration
- Custom Tags & Labels
- Bonus Features Tracking
- TMDb Automatic Enrichment (VERIFIED IN CODEBASE)
- Collection Editing
- Title Locking
- Security Hardening

**📋 REMAINING FEATURES**: 19 features across 4 priority tiers

---

## Missing Features Summary

### From ROADMAP.md (Phase 2-4)

| Feature | Phase | Effort | Value | Status |
|---------|-------|--------|-------|--------|
| IMDb Discovery/Recommendations | Phase 2 | 3-4 weeks | High | ❌ Not Implemented |
| Multi-Site Expansion (Coolblue, MediaMarkt) | Phase 2 | 2-3 weeks/site | High | ❌ Not Implemented |
| Custom Alert Rules | Phase 2 | 3 weeks | High | ❌ Not Implemented |
| Inventory Management (Barcode) | Phase 3 | 3 weeks | Medium-High | ❌ Not Implemented |
| Backup & Cloud Sync | Phase 3 | 4 weeks | High | ❌ Not Implemented |
| Progressive Web App (PWA) | Phase 3 | 4-5 weeks | Medium | ❌ Not Implemented |
| Social Sharing (X/Twitter) | Phase 4 | 2-3 weeks | Low-Medium | ❌ Not Implemented |

### From FEATURES.md (New Ideas)

| Feature | Effort | Value | Status |
|---------|--------|-------|--------|
| Deals Page / Deal Finder | 3-4 weeks | High | ❌ Not Implemented |
| Audio & Subtitle Tracks | 2-3 weeks | Medium-High | ❌ Not Implemented |
| Card-Based UI Overhaul | 3-4 weeks | High | ❌ Not Implemented |
| Blu-ray Player Tracking (Tweakers) | 3-4 weeks | Medium | ❌ Not Implemented |
| Advanced Search & Filtering | 2-3 weeks | Medium-High | ❌ Not Implemented |
| Statistics & Analytics Dashboard | 2-3 weeks | Medium | ❌ Not Implemented |
| Smart Notifications & Digest Emails | 2-3 weeks | Medium | ❌ Not Implemented |
| Import/Export Functionality | 2 weeks | Medium | ❌ Not Implemented |
| Multi-Language Support (i18n) | 3 weeks | Low-Medium | ❌ Not Implemented |
| External Integrations (Letterboxd, Trakt) | Varies | Low-Medium | ❌ Not Implemented |
| User Accounts & Multi-User | 6+ weeks | Low | ❌ Not Implemented |
| Price Prediction (ML) | High | Low | ❌ Not Implemented |

---

## Prioritized Implementation Plan

### Priority Scoring Formula
**Score = (Value × User Impact) / (Effort × Risk)**

Where:
- Value: 1-5 (feature value to users)
- User Impact: 1-3 (multiplier for user benefit)
- Effort: 1-5 (weeks of work)
- Risk: 1-3 (technical complexity/dependencies)

---

## 🚀 Tier 1: High Impact, Low-Medium Effort (Next 8-12 Weeks)

### 1. ⭐ Card-Based UI Overhaul
**Priority Score**: 3.75
**Effort**: 3-4 weeks
**Value**: Very High
**Risk**: Low (frontend only)
**Dependencies**: None

#### Why This First?
- Dramatically improves UX without backend changes
- Addresses #1 user request from competitive analysis
- Makes the app visually competitive with CLZ Movies, iCollect
- Foundation for future features (drag-drop, visual sorting)

#### Implementation Tasks

**Week 1: UI Component Design**
1. Create responsive grid layout (CSS Grid)
2. Design movie card component with hover effects
3. Implement image lazy loading
4. Add skeleton loading states

**Week 2: View Toggle & State Management**
1. Build view switcher (Table/Card/List)
2. Persist preference in localStorage
3. Implement card interactions (click, hover)
4. Add quick action buttons (Edit, Delete, Trailer)

**Week 3: Advanced Features**
1. Implement drag-and-drop reordering
2. Add bulk selection mode
3. Create filter chips UI
4. Optimize rendering for 500+ items

**Week 4: Polish & Testing**
1. Responsive design for mobile/tablet
2. Keyboard navigation (arrow keys)
3. Accessibility (ARIA labels, focus management)
4. Cross-browser testing

#### Success Metrics
- [ ] Page loads <2s for 100 items
- [ ] 70%+ users prefer card view over table
- [ ] Full keyboard navigation support
- [ ] Mobile-responsive on all breakpoints

---

### 2. 💰 Deals Page / Deal Finder
**Priority Score**: 3.33
**Effort**: 3-4 weeks
**Value**: Very High
**Risk**: Medium (external scraping)
**Dependencies**: Existing scrapers

#### Why This Matters?
- Proactive value proposition (users come back daily)
- Leverages existing price history infrastructure
- Differentiator from competitors
- Drives user engagement

#### Implementation Tasks

**Week 1: Deal Detection Algorithm**
1. Define deal criteria (% discount, historical low, below target)
2. Create `DealsService` class in `src/application/deals/`
3. Implement deal scoring algorithm
4. Add database view for active deals

**Week 2: External Deals Scraping**
1. Scrape Amazon.nl "Deals" page
2. Scrape Bol.com "Aanbiedingen"
3. Parse deal metadata (end time, discount %)
4. Store in `deals` table

**Week 3: Frontend Implementation**
1. Create `/deals` route and page
2. Design deal card with countdown timer
3. Implement filters (format, source, min discount)
4. Add "Add to Wishlist" quick action

**Week 4: Real-time Updates & Notifications**
1. WebSocket broadcasts for new deals
2. Toast notifications for lightning deals
3. Discord/Email notifications for wishlist deals
4. Scheduled scraping via cron

#### Database Schema
```sql
CREATE TABLE deals (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    url TEXT NOT NULL UNIQUE,
    title TEXT NOT NULL,
    source TEXT NOT NULL,
    original_price REAL NOT NULL,
    deal_price REAL NOT NULL,
    discount_percentage REAL NOT NULL,
    deal_type TEXT, -- 'lightning', 'daily', 'promotion'
    ends_at TEXT,
    is_uhd_4k INTEGER DEFAULT 0,
    image_url TEXT,
    discovered_at TEXT NOT NULL,
    last_checked TEXT NOT NULL,
    is_active INTEGER DEFAULT 1
);
```

#### Success Metrics
- [ ] 10+ active deals at any time
- [ ] Deal detection accuracy >85%
- [ ] 30% of users visit deals page weekly
- [ ] <5% false positives

---

### 3. 🔍 Advanced Search & Filtering
**Priority Score**: 3.0
**Effort**: 2-3 weeks
**Value**: High
**Risk**: Low
**Dependencies**: None

#### Implementation Tasks

**Week 1: Query Parser**
1. Build search query parser (title:, price:, year:, rating:)
2. Implement operator support (<, >, =, range)
3. Create SQL query builder from parsed criteria
4. Add full-text search index on titles

**Week 2: Saved Searches**
1. Database schema for saved searches
2. API endpoints (CRUD)
3. "Save Search" UI button
4. Notifications for new matches

**Week 3: UI Components**
1. Advanced search modal
2. Filter chips display
3. Autocomplete for titles/directors
4. Recent searches dropdown

#### Database Schema
```sql
CREATE TABLE saved_searches (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    query TEXT NOT NULL,
    notify_on_new INTEGER DEFAULT 0,
    created_at TEXT NOT NULL
);

-- Full-text search index
CREATE VIRTUAL TABLE wishlist_fts USING fts5(
    title, content='wishlist', content_rowid='id'
);
```

#### Success Metrics
- [ ] Search response time <100ms
- [ ] Support 10+ search operators
- [ ] 20% of users save at least 1 search

---

### 4. 📊 Statistics & Analytics Dashboard
**Priority Score**: 2.67
**Effort**: 2-3 weeks
**Value**: Medium-High
**Risk**: Low
**Dependencies**: Price history, TMDb metadata

#### Implementation Tasks

**Week 1: Backend Analytics**
1. Create `AnalyticsService` class
2. Calculate collection statistics (value, spending)
3. Aggregate price trends
4. Genre distribution queries

**Week 2: Chart.js Visualizations**
1. Collection value over time (line chart)
2. Genre distribution (pie chart)
3. Monthly spending (bar chart)
4. Activity heatmap (calendar view)

**Week 3: Dashboard Page**
1. Create `/analytics` route
2. Design widget grid layout
3. Add export to PDF functionality
4. Implement date range filters

#### Statistics to Display
- **Collection Overview**
  - Total items (4K vs Blu-ray)
  - Total current value
  - Total spent vs current value
  - Average price per item
  - Most expensive item
  - Best deal (biggest savings)

- **Price Trends**
  - Average 4K price over time
  - Price changes correlation with seasons
  - Per-retailer price comparison

- **Genre Analysis**
  - Genre distribution (pie chart)
  - Most collected directors/actors
  - Rating distribution

- **Activity Timeline**
  - Items added per month
  - Purchase patterns
  - Wishlist conversion rate

#### Success Metrics
- [ ] 10+ unique statistics displayed
- [ ] Charts render <1s
- [ ] Export functionality works

---

### 5. 🎨 Audio & Subtitle Tracks Information
**Priority Score**: 2.67
**Effort**: 2-3 weeks
**Value**: Medium-High
**Risk**: Medium (external scraping)
**Dependencies**: Blu-ray.com scraper

#### Implementation Tasks

**Week 1: Database Schema & Models**
1. Create `disc_specs` table
2. Add domain models for audio/subtitle tracks
3. Implement repository pattern

**Week 2: Blu-ray.com Scraper**
1. Build scraper for technical specs
2. Parse audio formats (Dolby Atmos, DTS:X)
3. Extract subtitle languages
4. Store HDR formats, region codes

**Week 3: Frontend Display**
1. Audio badges UI (🔊 icons)
2. Subtitle flags (🇳🇱, 🇬🇧)
3. Expandable specs section in modal
4. Filter by audio format

#### Database Schema
```sql
CREATE TABLE disc_specs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    wishlist_id INTEGER,
    collection_id INTEGER,
    audio_tracks TEXT, -- JSON: [{"language": "English", "format": "Dolby Atmos"}]
    subtitle_tracks TEXT, -- JSON: ["English", "Dutch", "French"]
    video_resolution TEXT, -- "4K UHD", "1080p"
    hdr_format TEXT, -- "Dolby Vision", "HDR10+"
    region_code TEXT, -- "A", "B", "C", "Free"
    runtime_minutes INTEGER,
    FOREIGN KEY (wishlist_id) REFERENCES wishlist(id),
    FOREIGN KEY (collection_id) REFERENCES collection(id)
);
```

#### Success Metrics
- [ ] 50%+ items have audio/subtitle data
- [ ] Filter by audio format works
- [ ] Display accuracy >90%

---

## 🎯 Tier 2: High Value, Medium-High Effort (Weeks 13-24)

### 6. 🤖 IMDb Discovery/Recommendations
**Priority Score**: 2.5
**Effort**: 3-4 weeks
**Value**: High
**Risk**: Medium
**Dependencies**: TMDb enrichment (✅ Complete)

#### Implementation Tasks

**Week 1-2: Recommendation Engine**
1. Create `RecommendationEngine` class
2. Use TMDb `/movie/{id}/recommendations` API
3. Implement scoring algorithm (genre overlap, rating proximity)
4. Filter duplicates (already in collection/wishlist)

**Week 3: Availability Check**
1. Cross-check recommendations with Amazon.nl/Bol.com
2. Search for exact matches
3. Cache availability status

**Week 4: Frontend Page**
1. Create `/discover` route
2. Design recommendation cards
3. "Add to Wishlist" integration
4. "Refresh Recommendations" button

#### Success Metrics
- [ ] 20 personalized recommendations
- [ ] 60%+ users find 1+ interesting title
- [ ] 5% conversion to wishlist additions

---

### 7. 🛒 Multi-Site Expansion (Coolblue, MediaMarkt)
**Priority Score**: 2.67
**Effort**: 2-3 weeks per site
**Value**: High
**Risk**: High (anti-scraping)
**Dependencies**: Scraper factory pattern

#### Implementation Tasks (Per Site)

**Week 1: Scraper Development**
1. Create `CoolblueNlScraper` or `MediaMarktNlScraper`
2. Analyze HTML structure
3. Implement price/stock parsing
4. Handle JavaScript-rendered content (if needed)

**Week 2: Integration**
1. Register in `ScraperFactory`
2. Add source badge in frontend
3. Configure per-site delays
4. Test with real product URLs

**Week 3: Anti-Scraping Mitigation**
1. User-agent rotation
2. Proxy support (if needed)
3. Cloudflare bypass (if needed)
4. Fallback to manual entry

#### Challenges
- **Coolblue**: Likely has Cloudflare protection
- **MediaMarkt**: May use JavaScript rendering (needs headless browser)

#### Alternative Approach
- Check for official APIs (developer portals)
- Partner integrations (if available)

#### Success Metrics (Per Site)
- [ ] 90%+ scraping success rate
- [ ] No bans after 1000 requests
- [ ] Price accuracy >95%

---

### 8. 🔔 Custom Alert Rules
**Priority Score**: 2.67
**Effort**: 3 weeks
**Value**: High
**Risk**: Low
**Dependencies**: None

#### Implementation Tasks

**Week 1: Rule Engine**
1. Create `RuleEngine` class
2. Design JSON-based DSL for rules
3. Implement condition evaluator
4. Add rule validation

**Week 2: Database & API**
1. Create `alert_rules` table
2. Repository pattern implementation
3. REST API endpoints (CRUD)
4. Integrate with scheduler

**Week 3: Frontend**
1. Visual rule builder UI
2. Condition dropdowns (field, operator, value)
3. Preview matching items
4. Enable/disable toggles

#### Rule DSL Example
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

#### Success Metrics
- [ ] Users create 2+ custom rules on average
- [ ] Zero false positives
- [ ] Rule evaluation <50ms

---

### 9. 💾 Import/Export Functionality
**Priority Score**: 3.0
**Effort**: 2 weeks
**Value**: Medium
**Risk**: Low
**Dependencies**: None

#### Implementation Tasks

**Week 1: Export Formats**
1. CSV export (wishlist, collection)
2. JSON export (full backup)
3. PDF export (printable list)
4. HTML export (shareable static page)

**Week 2: Import Parsers**
1. CSV import with mapping UI
2. CLZ Movies import (analyze format)
3. Manual bulk add (paste URLs)
4. Duplicate detection

#### Success Metrics
- [ ] Support 3+ export formats
- [ ] Support 2+ import sources
- [ ] Import accuracy >95%

---

## 🔧 Tier 3: Infrastructure & Advanced (Weeks 25-40)

### 10. 📱 Progressive Web App (PWA)
**Effort**: 4-5 weeks
**Value**: Medium

#### Tasks
- Service worker for offline support
- App manifest
- Push notifications API
- Installable on mobile

---

### 11. 🔐 Backup & Cloud Sync
**Effort**: 4 weeks
**Value**: High

#### Tasks
- SQLite backup service
- AES-256 encryption
- Dropbox/Google Drive integration
- Restore functionality

---

### 12. 📺 Blu-ray Player Tracking (Tweakers)
**Effort**: 3-4 weeks
**Value**: Medium

#### Tasks
- Hardware table schema
- Tweakers.net scraper
- Price history for hardware
- Specs tracking

---

### 13. 📦 Inventory Management (Barcode)
**Effort**: 3 weeks
**Value**: Medium-High

#### Tasks
- Camera access (MediaDevices API)
- ZXing-JS barcode library
- UPC database lookup
- Auto-fill form from barcode

---

### 14. 🔔 Smart Notifications & Digest Emails
**Effort**: 2-3 weeks
**Value**: Medium

#### Tasks
- Daily digest aggregation
- Weekly newsletter
- Notification batching
- Preference management

---

## 🌐 Tier 4: Nice-to-Have (Future)

### 15. Multi-Language Support (i18n)
**Effort**: 3 weeks | **Value**: Low-Medium

### 16. External Integrations (Letterboxd, Trakt)
**Effort**: Varies | **Value**: Low-Medium

### 17. Social Sharing (X/Twitter)
**Effort**: 2-3 weeks | **Value**: Low-Medium

### 18. User Accounts & Multi-User
**Effort**: 6+ weeks | **Value**: Low (major architecture change)

### 19. Price Prediction (ML)
**Effort**: High | **Value**: Low

---

## Implementation Timeline

### Quarter 1 (Weeks 1-12) - Tier 1
- ✅ **Weeks 1-4**: Card-Based UI Overhaul
- ✅ **Weeks 5-8**: Deals Page / Deal Finder
- ✅ **Weeks 9-10**: Advanced Search & Filtering
- ✅ **Weeks 11-12**: Statistics Dashboard

### Quarter 2 (Weeks 13-24) - Tier 2
- **Weeks 13-15**: Audio & Subtitle Tracks
- **Weeks 16-19**: IMDb Discovery/Recommendations
- **Weeks 20-22**: Custom Alert Rules
- **Weeks 23-24**: Import/Export

### Quarter 3 (Weeks 25-36) - Tier 3
- **Weeks 25-29**: Progressive Web App
- **Weeks 30-33**: Backup & Cloud Sync
- **Weeks 34-36**: Blu-ray Player Tracking

### Quarter 4 (Weeks 37+) - Tier 3 Continued
- **Weeks 37-39**: Inventory Management (Barcode)
- **Weeks 40-42**: Smart Notifications
- **Weeks 43-45**: Multi-Site Expansion (per site)

---

## Resource Requirements

### Development
- 1 Full-stack developer (C++ backend + JavaScript frontend)
- Estimated: 45 weeks (11 months) of work for all features

### Infrastructure
- TMDb API key (✅ already configured)
- Cloud storage account (for backups)
- Optional: Proxy service (for anti-scraping)

### Testing
- Manual testing on desktop/mobile
- Integration tests for new API endpoints
- Load testing for deals scraper

---

## Risk Mitigation

### High-Risk Items
1. **Multi-Site Scrapers**: Anti-scraping measures (Cloudflare, rate limits)
   - **Mitigation**: User-agent rotation, proxies, API alternatives
   - **Fallback**: Manual entry

2. **Blu-ray.com Scraping**: No official API
   - **Mitigation**: Respect robots.txt, conservative delays
   - **Fallback**: User-contributed data

3. **Cloud Sync Security**: Data privacy
   - **Mitigation**: End-to-end encryption, user-controlled keys
   - **Compliance**: GDPR compliance for EU users

### Low-Risk Items
- Card UI (frontend only, no breaking changes)
- Statistics Dashboard (read-only, uses existing data)
- Import/Export (isolated feature)

---

## Success Criteria

### Tier 1 Completion (Q1)
- [ ] Card view adoption >70%
- [ ] Deals page attracts 10+ daily deals
- [ ] Advanced search handles complex queries
- [ ] Analytics dashboard shows 10+ stats

### Tier 2 Completion (Q2)
- [ ] Recommendations page generates 20+ suggestions
- [ ] Custom alert rules have 0% false positives
- [ ] Import/Export supports 3+ formats

### Tier 3 Completion (Q3-Q4)
- [ ] PWA installable on mobile devices
- [ ] Automated cloud backups configured
- [ ] Hardware tracking for 20+ players

---

## Next Steps

### Immediate Actions (This Week)
1. ✅ Review this plan with stakeholders
2. ✅ Confirm Tier 1 priorities
3. Create feature branch: `feature/card-ui-overhaul`
4. Begin Card-Based UI implementation

### Sprint Planning
- **Sprint Duration**: 2 weeks
- **Planning**: Monday mornings
- **Progress Check**: Thursday afternoons
- **Demo**: Friday end-of-day

---

## Appendix A: Feature Dependency Graph

```
Phase 1 (✅ Complete)
└── TMDb Enrichment
    ├── Phase 2: IMDb Discovery (depends on TMDb)
    └── Phase 2: Audio/Subtitle Tracks (uses TMDb data)

Card-Based UI (Tier 1)
└── No dependencies (can start immediately)

Deals Page (Tier 1)
└── Uses existing price history infrastructure

Advanced Search (Tier 1)
└── No dependencies

Statistics Dashboard (Tier 1)
└── Uses price history + TMDb metadata

Custom Alert Rules (Tier 2)
└── Uses existing notification infrastructure

Multi-Site Expansion (Tier 2)
└── Uses existing scraper factory pattern

PWA (Tier 3)
└── Requires modern frontend architecture (Card UI helpful but not required)

Backup & Cloud Sync (Tier 3)
└── No dependencies

Import/Export (Tier 2)
└── No dependencies
```

---

**Document Version**: 1.0
**Last Updated**: 2026-01-22
**Maintained By**: Claude (AI Assistant)
**Review Cycle**: Bi-weekly

---

**END OF PLAN**
