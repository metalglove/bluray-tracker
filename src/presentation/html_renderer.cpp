#include "html_renderer.hpp"

namespace bluray::presentation {

std::string HtmlRenderer::renderSPA() {
  std::string html = renderHead();
  html += "<body data-theme=\"dark\">\n";
  html += renderSidebar();
  html += renderMainContent();
  html += renderModals();
  html += renderScripts();
  html += "</body>\n</html>";
  return html;
}

std::string HtmlRenderer::renderHead() {
  return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Blu-ray Tracker - Modern UI</title>
    <link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
)HTML" + renderStyles() +
         R"HTML(
</head>
)HTML";
}

std::string HtmlRenderer::renderStyles() {
  return R"HTML(
    <style>
        :root {
            --primary: #667eea;
            --primary-dark: #5568d3;
            --secondary: #764ba2;
            --success: #10b981;
            --danger: #ef4444;
            --warning: #f59e0b;
            --info: #3b82f6;
            --bg-primary: #0f172a;
            --bg-secondary: #1e293b;
            --bg-tertiary: #334155;
            --text-primary: #f1f5f9;
            --text-secondary: #cbd5e1;
            --text-muted: #94a3b8;
            --border: #334155;
            --shadow: rgba(0, 0, 0, 0.3);
            --gradient-primary: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            --gradient-success: linear-gradient(135deg, #10b981 0%, #059669 100%);
            --gradient-danger: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
        }

        [data-theme="light"] {
            --bg-primary: #f8fafc;
            --bg-secondary: #ffffff;
            --bg-tertiary: #f1f5f9;
            --text-primary: #0f172a;
            --text-secondary: #475569;
            --text-muted: #64748b;
            --border: #e2e8f0;
            --shadow: rgba(0, 0, 0, 0.1);
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: var(--bg-primary);
            color: var(--text-primary);
            line-height: 1.6;
            transition: all 0.3s ease;
        }

        /* Sidebar */
        .sidebar {
            position: fixed;
            top: 0;
            left: 0;
            width: 260px;
            height: 100vh;
            background: var(--bg-secondary);
            border-right: 1px solid var(--border);
            padding: 2rem 0;
            overflow-y: auto;
            z-index: 100;
            transition: transform 0.3s ease;
        }

        .sidebar-header {
            padding: 0 1.5rem 2rem;
            border-bottom: 1px solid var(--border);
        }

        .logo {
            font-size: 1.5rem;
            font-weight: 700;
            background: var(--gradient-primary);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        .nav-menu {
            padding: 1.5rem 0;
        }

        .nav-item {
            display: flex;
            align-items: center;
            gap: 0.75rem;
            padding: 0.875rem 1.5rem;
            color: var(--text-secondary);
            text-decoration: none;
            transition: all 0.2s ease;
            cursor: pointer;
            border-left: 3px solid transparent;
        }

        .nav-item:hover {
            background: var(--bg-tertiary);
            color: var(--text-primary);
        }

        .nav-item.active {
            background: var(--bg-tertiary);
            color: var(--primary);
            border-left-color: var(--primary);
            font-weight: 600;
        }

        .nav-icon {
            font-size: 1.25rem;
        }

        /* Main Content */
        .main-content {
            margin-left: 260px;
            min-height: 100vh;
            transition: margin-left 0.3s ease;
        }

        .header {
            background: var(--bg-secondary);
            border-bottom: 1px solid var(--border);
            padding: 1.25rem 2rem;
            display: flex;
            justify-content: space-between;
            align-items: center;
            position: sticky;
            top: 0;
            z-index: 90;
        }

        .header-title {
            font-size: 1.75rem;
            font-weight: 700;
        }

        .header-actions {
            display: flex;
            gap: 1rem;
            align-items: center;
        }

        .theme-toggle {
            width: 2.5rem;
            height: 2.5rem;
            border-radius: 50%;
            background: var(--bg-tertiary);
            border: none;
            color: var(--text-primary);
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 1.25rem;
            transition: all 0.2s ease;
        }

        .theme-toggle:hover {
            background: var(--primary);
            color: white;
            transform: rotate(20deg);
        }

        .container {
            padding: 2rem;
            max-width: 1600px;
        }

        /* Cards */
        .card {
            background: var(--bg-secondary);
            border-radius: 1rem;
            padding: 1.5rem;
            border: 1px solid var(--border);
            box-shadow: 0 4px 6px var(--shadow);
            transition: all 0.3s ease;
        }

        .card:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 12px var(--shadow);
        }

        .card-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1.5rem;
        }

        .card-title {
            font-size: 1.25rem;
            font-weight: 600;
        }

        /* Stats Grid */
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 1.5rem;
            margin-bottom: 2rem;
        }

        .stat-card {
            background: var(--gradient-primary);
            border-radius: 1rem;
            padding: 1.5rem;
            color: white;
            box-shadow: 0 4px 6px var(--shadow);
            transition: transform 0.3s ease;
        }

        .stat-card:hover {
            transform: translateY(-4px);
        }

        .stat-card.success {
            background: var(--gradient-success);
        }

        .stat-card.danger {
            background: var(--gradient-danger);
        }

        .stat-icon {
            font-size: 2.5rem;
            opacity: 0.9;
            margin-bottom: 0.5rem;
        }

        .stat-value {
            font-size: 2.5rem;
            font-weight: 700;
            margin-bottom: 0.25rem;
        }

        .stat-label {
            font-size: 0.875rem;
            opacity: 0.9;
        }

        /* Buttons */
        .btn {
            padding: 0.75rem 1.5rem;
            border-radius: 0.5rem;
            border: none;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s ease;
            display: inline-flex;
            align-items: center;
            gap: 0.5rem;
            font-size: 0.875rem;
        }

        .btn-primary {
            background: var(--gradient-primary);
            color: white;
        }

        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 8px var(--shadow);
        }

        .btn-success {
            background: var(--success);
            color: white;
        }

        .btn-danger {
            background: var(--danger);
            color: white;
        }

        .btn-secondary {
            background: var(--bg-tertiary);
            color: var(--text-primary);
        }

        .btn:disabled {
            opacity: 0.6;
            cursor: not-allowed;
            transform: none !important;
        }

        /* Table */
        .table-container {
            overflow-x: auto;
            border-radius: 0.75rem;
            border: 1px solid var(--border);
        }

        table {
            width: 100%;
            border-collapse: collapse;
        }

        th {
            background: var(--bg-tertiary);
            padding: 1rem;
            text-align: left;
            font-weight: 600;
            border-bottom: 1px solid var(--border);
            white-space: nowrap;
        }

        td {
            padding: 1rem;
            border-bottom: 1px solid var(--border);
        }

        tbody tr {
            transition: background 0.2s ease;
        }

        tbody tr:hover {
            background: var(--bg-tertiary);
        }

        .item-image {
            width: 60px;
            height: 90px;
            object-fit: cover;
            border-radius: 0.5rem;
            box-shadow: 0 2px 4px var(--shadow);
        }

        .table-image img {
            width: 100px;
            height: 150px;
            object-fit: cover;
            border-radius: 4px;
            box-shadow: 0 1px 2px var(--shadow);
        }

        .badge {
            display: inline-block;
            padding: 0.25rem 0.75rem;
            border-radius: 9999px;
            font-size: 0.75rem;
            font-weight: 600;
        }

        .badge-success {
            background: var(--success);
            color: white;
        }

        .badge-danger {
            background: var(--danger);
            color: white;
        }

        .badge-info {
            background: var(--info);
            color: white;
        }

        .badge-warning {
            background: var(--warning);
            color: white;
        }

        /* Modal */
        .modal {
            display: none;
            position: fixed;
            inset: 0;
            z-index: 1000;
            align-items: center;
            justify-content: center;
            background: rgba(0, 0, 0, 0.7);
            backdrop-filter: blur(4px);
            animation: fadeIn 0.2s ease;
        }

        .modal.active {
            display: flex;
        }

        .modal-content {
            background: var(--bg-secondary);
            border-radius: 1rem;
            padding: 2rem;
            max-width: 500px;
            width: 90%;
            max-height: 90vh;
            overflow-y: auto;
            box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.3);
            animation: slideUp 0.3s ease;
        }

        .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1.5rem;
        }

        .modal-title {
            font-size: 1.5rem;
            font-weight: 700;
        }

        .close-modal {
            width: 2rem;
            height: 2rem;
            border-radius: 50%;
            background: var(--bg-tertiary);
            border: none;
            color: var(--text-primary);
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 1.25rem;
        }

        .close-modal:hover {
            background: var(--danger);
            color: white;
        }

        /* Form */
        .form-group {
            margin-bottom: 1.5rem;
        }

        .form-label {
            display: block;
            margin-bottom: 0.5rem;
            font-weight: 600;
            color: var(--text-secondary);
        }

        .form-input {
            width: 100%;
            padding: 0.75rem;
            border: 1px solid var(--border);
            border-radius: 0.5rem;
            background: var(--bg-tertiary);
            color: var(--text-primary);
            font-size: 1rem;
            transition: all 0.2s ease;
        }

        .form-input:focus {
            outline: none;
            border-color: var(--primary);
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }

        .checkbox-group {
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        /* Toast */
        .toast-container {
            position: fixed;
            top: 2rem;
            right: 2rem;
            z-index: 2000;
            display: flex;
            flex-direction: column;
            gap: 1rem;
        }

        .toast {
            min-width: 300px;
            padding: 1rem 1.5rem;
            border-radius: 0.75rem;
            color: white;
            box-shadow: 0 10px 15px -3px var(--shadow);
            display: flex;
            align-items: center;
            gap: 0.75rem;
            animation: slideInRight 0.3s ease;
        }

        .toast-success {
            background: var(--success);
        }

        .toast-error {
            background: var(--danger);
        }

        .toast-info {
            background: var(--info);
        }

        .toast-icon {
            font-size: 1.5rem;
        }

        /* Loading */
        .loading {
            display: inline-block;
            width: 1rem;
            height: 1rem;
            border: 2px solid rgba(255, 255, 255, 0.3);
            border-top-color: white;
            border-radius: 50%;
            animation: spin 0.8s linear infinite;
        }

        /* Search & Filter */
        .search-bar {
            display: flex;
            gap: 1rem;
            margin-bottom: 1.5rem;
            flex-wrap: wrap;
        }

        .search-input {
            flex: 1;
            min-width: 250px;
        }

        .filter-select {
            padding: 0.75rem;
            border: 1px solid var(--border);
            border-radius: 0.5rem;
            background: var(--bg-tertiary);
            color: var(--text-primary);
            cursor: pointer;
        }

        /* Pagination */
        .pagination {
            display: flex;
            justify-content: center;
            gap: 0.5rem;
            margin-top: 2rem;
        }

        .page-btn {
            padding: 0.5rem 1rem;
            border: 1px solid var(--border);
            background: var(--bg-tertiary);
            color: var(--text-primary);
            border-radius: 0.5rem;
            cursor: pointer;
            transition: all 0.2s ease;
        }

        .page-btn:hover:not(:disabled) {
            background: var(--primary);
            color: white;
            border-color: var(--primary);
        }

        .page-btn.active {
            background: var(--primary);
            color: white;
            border-color: var(--primary);
        }

        .page-btn:disabled {
            opacity: 0.4;
            cursor: not-allowed;
        }

        /* Animations */
        @keyframes fadeIn {
            from { opacity: 0; }
            to { opacity: 1; }
        }

        @keyframes slideUp {
            from {
                opacity: 0;
                transform: translateY(20px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        @keyframes slideInRight {
            from {
                opacity: 0;
                transform: translateX(100px);
            }
            to {
                opacity: 1;
                transform: translateX(0);
            }
        }

        @keyframes spin {
            to { transform: rotate(360deg); }
        }

        /* Responsive */
        @media (max-width: 768px) {
            .sidebar {
                transform: translateX(-100%);
            }

            .sidebar.active {
                transform: translateX(0);
            }

            .main-content {
                margin-left: 0;
            }

            .stats-grid {
                grid-template-columns: 1fr;
            }

            .table-container {
                font-size: 0.875rem;
            }

            .container {
                padding: 1rem;
            }
        }

        /* Empty State */
        .empty-state {
            text-align: center;
            padding: 4rem 2rem;
        }

        .empty-icon {
            font-size: 4rem;
            opacity: 0.5;
            margin-bottom: 1rem;
        }

        .empty-title {
            font-size: 1.5rem;
            font-weight: 600;
            margin-bottom: 0.5rem;
        }

        .empty-text {
            color: var(--text-muted);
            margin-bottom: 1.5rem;
        }

        /* Connection Status */
        .connection-status {
            display: flex;
            align-items: center;
            gap: 0.5rem;
            font-size: 0.875rem;
            color: var(--text-muted);
        }

        .connection-dot {
            width: 0.5rem;
            height: 0.5rem;
            border-radius: 50%;
            background: var(--success);
            animation: pulse 2s infinite;
        }

        .connection-dot.disconnected {
            background: var(--danger);
            animation: none;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
    </style>
)HTML";
}

std::string HtmlRenderer::renderSidebar() {
  return R"HTML(
    <!-- Sidebar -->
    <aside class="sidebar" id="sidebar">
        <div class="sidebar-header">
            <div class="nav-brand">
                <span class="nav-icon">🎬</span>
                Blu-ray Tracker
            </div>
            
            <!-- Scrape Progress Indicator -->
            <div id="scrapeProgressContainer" style="flex: 1; max-width: 300px; margin: 0 2rem; display: none;">
                 <div style="display: flex; justify-content: space-between; font-size: 0.8rem; margin-bottom: 0.25rem;">
                    <span id="scrapeStatusText">Scraping...</span>
                    <span id="scrapePercent">0%</span>
                 </div>
                 <div style="height: 6px; background: var(--bg-tertiary); border-radius: 3px; overflow: hidden;">
                     <div id="scrapeProgressBar" style="height: 100%; width: 0%; background: var(--primary); transition: width 0.3s ease;"></div>
                 </div>
            </div>
        </div>

        <nav class="nav-links">
            <a class="nav-item active" data-page="dashboard">
                <span class="nav-icon">📊</span>
                <span>Dashboard</span>
            </a>
            <a class="nav-item" data-page="wishlist">
                <span class="nav-icon">⭐</span>
                <span>Wishlist</span>
            </a>
            <a class="nav-item" data-page="collection">
                <span class="nav-icon">📀</span>
                <span>Collection</span>
            </a>
            <a class="nav-item" data-page="settings">
                <span class="nav-icon">⚙️</span>
                <span>Settings</span>
            </a>
        </nav>
    </aside>
)HTML";
}

std::string HtmlRenderer::renderMainContent() {
  return R"HTML(
    <!-- Main Content -->
    <main class="main-content">
        <header class="header">
            <h1 class="header-title" id="pageTitle">Dashboard</h1>
            <div class="header-actions">
                <div class="connection-status">
                    <span class="connection-dot disconnected" id="wsDot"></span>
                    <span id="wsStatus">Connecting...</span>
                </div>
                <button class="theme-toggle" onclick="toggleTheme()">🌙</button>
            </div>
        </header>

        <div class="container">
            <!-- Dashboard Page -->
            <div id="dashboard-page" class="page-content">
                <div class="stats-grid" id="statsGrid">
                    <div class="stat-card">
                        <div class="stat-icon">⭐</div>
                        <div class="stat-value" id="wishlistCount">0</div>
                        <div class="stat-label">Wishlist Items</div>
                    </div>
                    <div class="stat-card success">
                        <div class="stat-icon">📀</div>
                        <div class="stat-value" id="collectionCount">0</div>
                        <div class="stat-label">Collection Items</div>
                    </div>
                    <div class="stat-card danger">
                        <div class="stat-icon">📦</div>
                        <div class="stat-value" id="inStockCount">0</div>
                        <div class="stat-label">In Stock</div>
                    </div>
                    <div class="stat-card" style="background: linear-gradient(135deg, #f59e0b 0%, #d97706 100%);">
                        <div class="stat-icon">🎞️</div>
                        <div class="stat-value" id="uhd4kCount">0</div>
                        <div class="stat-label">UHD 4K</div>
                    </div>
                </div>

                <div class="card">
                    <div class="card-header">
                        <h2 class="card-title">Quick Actions</h2>
                    </div>
                    <div style="display: flex; gap: 1rem; flex-wrap: wrap;">
                        <button class="btn btn-primary" onclick="openAddWishlistModal()">
                            <span>➕</span>
                            Add to Wishlist
                        </button>
                        <button class="btn btn-success" onclick="openAddCollectionModal()">
                            <span>📀</span>
                            Add to Collection
                        </button>
                        <button class="btn btn-secondary" onclick="scrapeNow()" id="scrapeBtn">
                            <span>🔄</span>
                            Scrape Now
                        </button>
                    </div>
                </div>
            </div>

            <!-- Wishlist Page -->
            <div id="wishlist-page" class="page-content" style="display: none;">
                <div class="card">
                    <div class="card-header">
                        <h2 class="card-title">Wishlist Management</h2>
                        <button class="btn btn-primary" onclick="openAddWishlistModal()">
                            <span>➕</span>
                            Add Item
                        </button>
                    </div>

                    <div class="search-bar">
                        <input type="text" class="form-input search-input" placeholder="Search titles..." id="wishlistSearch">
                        <select class="filter-select" id="wishlistSort">
                            <option value="date_desc">Newest First</option>
                            <option value="date_asc">Oldest First</option>
                            <option value="price_asc">Price Low-High</option>
                            <option value="price_desc">Price High-Low</option>
                            <option value="title_asc">Title A-Z</option>
                        </select>
                        <select class="filter-select" id="wishlistSourceFilter">
                            <option value="">All Sources</option>
                            <option value="amazon.nl">Amazon.nl</option>
                            <option value="bol.com">Bol.com</option>
                        </select>
                        <select class="filter-select" id="wishlistStockFilter">
                            <option value="">All Stock</option>
                            <option value="in_stock">In Stock</option>
                            <option value="out_of_stock">Out of Stock</option>
                        </select>
                    </div>

                    <div class="table-container">
                        <table>
                            <thead>
                                <tr>
                                    <th>Image</th>
                                    <th>Title</th>
                                    <th>Price</th>
                                    <th>Target</th>
                                    <th>Status</th>
                                    <th>Details</th>
                                    <th>Actions</th>
                                </tr>
                            </thead>
                            <tbody id="wishlistTableBody">
                                <!-- Populated by JS -->
                            </tbody>
                        </table>
                    </div>

                    <div class="pagination" id="wishlistPagination">
                        <!-- Populated by JS -->
                    </div>
                </div>
            </div>

            <!-- Collection Page -->
            <div id="collection-page" class="page-content" style="display: none;">
                <div class="card">
                    <div class="card-header">
                        <h2 class="card-title">My Collection</h2>
                        <div class="header-actions">
                            <span class="badge badge-info" id="collectionTotalValue" style="font-size: 1rem; margin-right: 1rem;">Total Value: €0.00</span>
                            <button class="btn btn-success" onclick="openAddCollectionModal()">
                                <span>📀</span>
                                Add Item
                            </button>
                        </div>
                    </div>

                    <div class="search-bar">
                        <input type="text" class="form-input search-input" placeholder="Search collection..." id="collectionSearch">
                         <select class="filter-select" id="collectionSourceFilter">
                            <option value="">All Sources</option>
                            <option value="amazon.nl">Amazon.nl</option>
                            <option value="bol.com">Bol.com</option>
                        </select>
                    </div>

                    <div class="table-container">
                        <table>
                            <thead>
                                <tr>
                                    <th>Image</th>
                                    <th>Title</th>
                                    <th>Format</th>
                                    <th>Price Paid</th>
                                    <th>Date Added</th>
                                    <th>Actions</th>
                                </tr>
                            </thead>
                            <tbody id="collectionTableBody">
                                <!-- Populated by JS -->
                            </tbody>
                        </table>
                    </div>

                    <div class="pagination" id="collectionPagination">
                        <!-- Populated by JS -->
                    </div>
                </div>
            </div>

            <!-- Settings Page -->
            <div id="settings-page" class="page-content" style="display: none;">
                <div class="card">
                    <div class="card-header">
                        <h2 class="card-title">Settings</h2>
                    </div>
                    <form id="settingsForm" onsubmit="saveSettings(event)">
                        <div class="form-group">
                            <label class="form-label">Scrape Delay (seconds)</label>
                            <input type="number" class="form-input" id="scrapeDelay" min="1" max="60" required>
                        </div>
                        <div class="form-group">
                            <label class="form-label">Cache Directory</label>
                            <input type="text" class="form-input" id="cacheDir" required>
                        </div>
                        <button type="submit" class="btn btn-primary">Save Settings</button>
                    </form>
                </div>
            </div>
        </div>
    </main>
)HTML";
}

std::string HtmlRenderer::renderModals() {
  return R"HTML(
    <!-- Modals -->
    
    <!-- Add Wishlist Modal -->
    <div class="modal" id="addWishlistModal">
        <div class="modal-content">
            <div class="modal-header">
                <h3 class="modal-title">Add to Wishlist</h3>
                <button class="close-modal" onclick="closeModal('addWishlistModal')">×</button>
            </div>
            <form id="addWishlistForm" onsubmit="addWishlistItem(event)">
                <div class="form-group">
                    <label class="form-label">URL</label>
                    <input type="url" class="form-input" id="wishlistUrl" placeholder="https://..." required>
                </div>
                <!-- Title removed, will be scraped -->
                <div class="form-group">
                    <label class="form-label">Desired Max Price (€)</label>
                    <input type="number" class="form-input" id="wishlistPrice" step="0.01" min="0" required>
                </div>
                <div class="form-group">
                    <label class="checkbox-group">
                        <input type="checkbox" id="notifyPrice" checked>
                        <span>Notify on price drop</span>
                    </label>
                </div>
                <div class="form-group">
                    <label class="checkbox-group">
                        <input type="checkbox" id="notifyStock" checked>
                        <span>Notify when in stock</span>
                    </label>
                </div>
                <button type="submit" class="btn btn-primary" style="width: 100%;">Add Item</button>
            </form>
        </div>
    </div>

    <!-- Edit Wishlist Modal -->
    <div class="modal" id="editWishlistModal">
        <div class="modal-content">
            <div class="modal-header">
                <h3 class="modal-title">Edit Wishlist Item</h3>
                <button class="close-modal" onclick="closeModal('editWishlistModal')">×</button>
            </div>
            <form id="editWishlistForm" onsubmit="updateWishlistItem(event)">
                <input type="hidden" id="editWishlistId">
                <div class="form-group">
                    <label class="form-label">Title</label>
                    <input type="text" class="form-input" id="editWishlistTitle" required>
                    <div style="margin-top: 5px;">
                        <label class="checkbox-group">
                            <input type="checkbox" id="editWishlistTitleLocked">
                            <span style="font-size: 0.9em; color: var(--text-muted);">Lock Title (prevent scraper updates)</span>
                        </label>
                    </div>
                </div>
                <div class="form-group">
                    <label class="form-label">Desired Max Price (€)</label>
                    <input type="number" class="form-input" id="editWishlistPrice" step="0.01" min="0" required>
                </div>
                <div class="form-group">
                    <label class="checkbox-group">
                        <input type="checkbox" id="editWishlistNotifyPrice">
                        <span>Notify on price drop</span>
                    </label>
                </div>
                <div class="form-group">
                    <label class="checkbox-group">
                        <input type="checkbox" id="editWishlistNotifyStock">
                        <span>Notify when in stock</span>
                    </label>
                </div>
                <button type="submit" class="btn btn-primary" style="width: 100%;">Update Item</button>
            </form>
            <div style="margin-top: 1rem; padding-top: 1rem; border-top: 1px solid var(--border);">
                 <button type="button" class="btn btn-danger" style="width: 100%;" onclick="deleteWishlistItem(document.getElementById('editWishlistId').value)">
                    Delete Item
                </button>
            </div>
        </div>
    </div>

    <!-- Add Collection Modal -->
    <div class="modal" id="addCollectionModal">
        <div class="modal-content">
            <div class="modal-header">
                <h3 class="modal-title">Add to Collection</h3>
                <button class="close-modal" onclick="closeModal('addCollectionModal')">×</button>
            </div>
            <form id="addCollectionForm" onsubmit="addCollectionItem(event)">
                <div class="form-group">
                    <label class="form-label">Title</label>
                    <input type="text" class="form-input" id="collectionTitle" required>
                </div>
                <div class="form-group">
                    <label class="form-label">Barcode (EAN/UPC) - Optional</label>
                    <input type="text" class="form-input" id="collectionBarcode">
                </div>
                <div class="form-group">
                    <label class="checkbox-group">
                        <input type="checkbox" id="collectionIsUHD">
                        <span>UHD 4K</span>
                    </label>
                </div>
                <button type="submit" class="btn btn-primary" style="width: 100%;">Add Item</button>
            </form>
        </div>
    </div>
    
    <!-- Edit Collection Modal -->
    <div class="modal" id="editCollectionModal">
        <div class="modal-content">
            <div class="modal-header">
                <h3 class="modal-title">Edit Collection Item</h3>
                <button class="close-modal" onclick="closeModal('editCollectionModal')">×</button>
            </div>
            <form id="editCollectionForm" onsubmit="updateCollectionItem(event)">
                <input type="hidden" id="editCollectionId">
                <div class="form-group">
                    <label class="form-label">Title</label>
                    <input type="text" class="form-input" id="editCollectionTitle" required>
                </div>
                 <div class="form-group">
                    <label class="form-label">Purchase Price (€)</label>
                    <input type="number" class="form-input" id="editCollectionPrice" step="0.01" min="0" required>
                </div>
                <div class="form-group">
                    <label class="checkbox-group">
                        <input type="checkbox" id="editCollectionIsUHD">
                        <span>UHD 4K</span>
                    </label>
                </div>
                <div class="form-group">
                    <label class="form-label">Notes</label>
                    <textarea class="form-input" id="editCollectionNotes" rows="3"></textarea>
                </div>
                <button type="submit" class="btn btn-primary" style="width: 100%;">Update Item</button>
            </form>
        </div>
    </div>

    <!-- Price History Modal -->
    <div class="modal" id="priceHistoryModal">
        <div class="modal-content" style="max-width: 800px;">
            <div class="modal-header">
                <h3 class="modal-title" id="historyTitle">Price History</h3>
                <button class="close-modal" onclick="closeModal('priceHistoryModal')">×</button>
            </div>
            <div style="height: 400px; position: relative;">
                <canvas id="priceHistoryChart"></canvas>
            </div>
        </div>
    </div>

    <!-- Toast Container -->
    <div class="toast-container" id="toastContainer"></div>
)HTML";
}

std::string HtmlRenderer::renderScripts() {
  return R"HTML(
    <script>
        // State
        let currentPage = 'dashboard';
        let wishlistData = { items: [], page: 1, total_pages: 1 };
        let collectionData = { items: [], page: 1, total_pages: 1 };
        let ws = null;
        let chartInstance = null;

        // Initialize
        document.addEventListener('DOMContentLoaded', () => {
            setupNavigation();
            setupWebSocket();
            loadDashboardStats();
            
            // Check params
            const urlParams = new URLSearchParams(window.location.search);
            const page = urlParams.get('page');
            if (page) navigateTo(page);

            // Add filter listeners
            document.getElementById('wishlistSort')?.addEventListener('change', () => loadWishlist(1));
            document.getElementById('wishlistStockFilter')?.addEventListener('change', () => loadWishlist(1));
            document.getElementById('wishlistSourceFilter')?.addEventListener('change', () => loadWishlist(1));
            document.getElementById('wishlistSearch')?.addEventListener('input', debounce(() => loadWishlist(1), 300));

            document.getElementById('collectionSourceFilter')?.addEventListener('change', () => loadCollection(1));
            document.getElementById('collectionSearch')?.addEventListener('input', debounce(() => loadCollection(1), 300));
        });

        function debounce(func, wait) {
            let timeout;
            return function(...args) {
                clearTimeout(timeout);
                timeout = setTimeout(() => func.apply(this, args), wait);
            };
        }

        // Navigation
        function setupNavigation() {
            document.querySelectorAll('.nav-item').forEach(item => {
                item.addEventListener('click', (e) => {
                    e.preventDefault();
                    navigateTo(item.dataset.page);
                });
            });
        }

        function navigateTo(page) {
            // Update UI
            document.querySelectorAll('.nav-item').forEach(item => {
                item.classList.toggle('active', item.dataset.page === page);
            });
            
            document.querySelectorAll('.page-content').forEach(content => {
                content.style.display = 'none';
            });
            
            document.getElementById(`${page}-page`).style.display = 'block';
            
            // Update Title
            const titles = {
                dashboard: 'Dashboard',
                wishlist: 'Wishlist',
                collection: 'My Collection',
                settings: 'Settings'
            };
            document.getElementById('pageTitle').textContent = titles[page];
            
            // Load Data
            if (page === 'dashboard') loadDashboardStats();
            if (page === 'wishlist') loadWishlist();
            if (page === 'collection') loadCollection();
            if (page === 'settings') loadSettings();
            
            currentPage = page;
            
            // Mobile sidebar
            if (window.innerWidth <= 768) {
                document.getElementById('sidebar').classList.remove('active');
            }
        }

        // WebSocket
        function setupWebSocket() {
            const proto = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const url = `${proto}//${window.location.host}/ws`;
            
            function connect() {
                ws = new WebSocket(url);
                
                ws.onopen = () => {
                    document.getElementById('wsDot').classList.remove('disconnected');
                    document.getElementById('wsStatus').textContent = 'Connected';
                };
                
                ws.onclose = () => {
                    document.getElementById('wsDot').classList.add('disconnected');
                    document.getElementById('wsStatus').textContent = 'Disconnected';
                    setTimeout(connect, 2000);
                };
                
                ws.onmessage = (event) => {
                    const msg = JSON.parse(event.data);
                    handleWebSocketMessage(msg);
                };
            }
            
            connect();
        }

        function handleWebSocketMessage(msg) {
            if (msg.type.startsWith('wishlist_')) {
                if (currentPage === 'wishlist') loadWishlist(wishlistData.page);
                loadDashboardStats();
                if (msg.message) {
                    showToast(msg.message, 'info');
                }
            }
        }

        // Data Loading
        async function loadDashboardStats() {
            try {
                const res = await fetch('/api/stats');
                const data = await res.json();

                document.getElementById('wishlistCount').textContent = data.wishlist_count;
                document.getElementById('collectionCount').textContent = data.collection_count;
                document.getElementById('inStockCount').textContent = data.in_stock_count;
                document.getElementById('uhd4kCount').textContent = data.uhd_4k_count;
                
                // Update Progress Bar
                const progressContainer = document.getElementById('scrapeProgressContainer');
                if (data.scraping_active) {
                    progressContainer.style.display = 'block';
                    const processed = data.scrape_progress.processed;
                    const total = data.scrape_progress.total;
                    const pct = total > 0 ? Math.round((processed / total) * 100) : 0;
                    
                    document.getElementById('scrapeProgressBar').style.width = `${pct}%`;
                    document.getElementById('scrapePercent').textContent = `${pct}%`;
                    document.getElementById('scrapeStatusText').textContent = `Scraping: ${processed}/${total}`;
                    
                    // Poll faster if scraping
                    setTimeout(loadDashboardStats, 1000);
                } else {
                    progressContainer.style.display = 'none';
                    // Re-enable button if it was disabled manually
                    const btn = document.getElementById('scrapeBtn');
                    if(btn && btn.disabled) {
                        btn.disabled = false;
                        btn.innerHTML = '<span>🔄</span> Scrape Now';
                        if (currentPage === 'wishlist') loadWishlist(wishlistData.page);
                        showToast('Scrape completed', 'success');
                    }
                }

            } catch (error) {
                console.error('Failed to load stats:', error);
            }
        }

        async function loadWishlist(page = 1) {
            try {
                const sortVal = document.getElementById('wishlistSort').value;
                const stockVal = document.getElementById('wishlistStockFilter').value;

                const sourceVal = document.getElementById('wishlistSourceFilter').value;
                const searchVal = document.getElementById('wishlistSearch').value;
                
                let sort = 'date';
                let order = 'desc';
                
                if (sortVal) {
                    const parts = sortVal.split('_');
                    if (parts.length === 2) {
                        sort = parts[0];
                        order = parts[1];
                    }
                }

                let url = `/api/wishlist?page=${page}&size=20&sort=${sort}&order=${order}`;
                if (stockVal) url += `&stock=${stockVal}`;
                if (sourceVal) url += `&source=${sourceVal}`;
                if (searchVal) url += `&search=${encodeURIComponent(searchVal)}`;

                const res = await fetch(url);
                wishlistData = await res.json();
                renderWishlistTable();
                renderWishlistPagination();
            } catch (error) {
                console.error('Failed to load wishlist:', error);
                showToast('Failed to load wishlist', 'error');
            }
        }

        function renderWishlistTable() {
            const tbody = document.getElementById('wishlistTableBody');
            
            if (wishlistData.items.length === 0) {
                tbody.innerHTML = `
                    <tr>
                        <td colspan="7" class="empty-state">
                            <div class="empty-icon">⭐</div>
                            <div class="empty-title">No items found</div>
                            <div class="empty-text">Add items to your wishlist to get started</div>
                        </td>
                    </tr>
                `;
                return;
            }

            tbody.innerHTML = wishlistData.items.map(item => `
                <tr>
                    <td>
                        <img src="${item.local_image_path || item.image_url || '/static/placeholder.png'}" 
                             class="item-image" onerror="this.src='https://placehold.co/60x90?text=No+Img'">
                    </td>
                    <td>
                        <div style="font-weight: 600;">${item.title || 'Loading title...'}</div>
                        <div style="font-size: 0.8rem; color: var(--text-muted);">
                            <a href="${item.url}" target="_blank" style="color: var(--primary); text-decoration: none;">
                                🔗 ${new URL(item.url).hostname}
                            </a>
                        </div>
                    </td>
                    <td>
                        <div class="stat-value" style="font-size: 1.2rem;">€${item.current_price.toFixed(2)}</div>
                    </td>
                    <td>
                        <div style="font-size: 0.9rem;">
                            <span style="color: ${item.current_price <= item.desired_max_price ? 'var(--success)' : 'var(--text-muted)'}">
                                Target: €${item.desired_max_price.toFixed(2)}
                            </span>
                        </div>
                    </td>
                    <td>
                        <span class="badge ${item.in_stock ? 'badge-success' : 'badge-danger'}">
                            ${item.in_stock ? 'In Stock' : 'Out of Stock'}
                        </span>
                    </td>
                     <td>
                        <div style="display: flex; gap: 0.25rem; flex-wrap: wrap;">
                             ${item.is_uhd_4k ? '<span class="badge badge-warning">4K</span>' : ''}
                        </div>
                    </td>
                    <td>
                        <div style="display: flex; gap: 0.5rem;">
                            <button class="btn btn-secondary" onclick="openEditWishlistModal(${item.id})">✏️</button>
                             <button class="btn btn-info" onclick="openPriceHistory(${item.id})">📈</button>
                            <button class="btn btn-danger" onclick="deleteWishlistItem(${item.id})">🗑️</button>
                        </div>
                    </td>
                </tr>
            `).join('');
        }

        function renderWishlistPagination() {
            const container = document.getElementById('wishlistPagination');
            // Simplified pagination
            container.innerHTML = `
                <button class="page-btn" ${!wishlistData.has_previous ? 'disabled' : ''} 
                        onclick="loadWishlist(${wishlistData.page - 1})">Previous</button>
                <span style="display: flex; align-items: center; color: var(--text-secondary);">
                    Page ${wishlistData.page} of ${wishlistData.total_pages}
                </span>
                <button class="page-btn" ${!wishlistData.has_next ? 'disabled' : ''} 
                        onclick="loadWishlist(${wishlistData.page + 1})">Next</button>
            `;
        }
        
        async function loadCollection(page = 1) {
            try {
                 const sourceVal = document.getElementById('collectionSourceFilter').value;
                 const searchVal = document.getElementById('collectionSearch').value;
                 let url = `/api/collection?page=${page}&size=20`;
                 if (sourceVal) url += `&source=${sourceVal}`;
                 if (searchVal) url += `&search=${encodeURIComponent(searchVal)}`;

                const res = await fetch(url);
                collectionData = await res.json();
                renderCollectionTable();
                updateCollectionTotal(collectionData.total_value);
                //renderCollectionPagination(); // TODO
            } catch (error) {
                 console.error('Failed to load collection:', error);
                 showToast('Failed to load collection', 'error');
            }
        }
        
        function updateCollectionTotal(value) {
            const el = document.getElementById('collectionTotalValue');
            if(el) {
                el.textContent = `Total Value: €${(value || 0).toFixed(2)}`;
            }
        }

        function renderCollectionTable() {
             const tbody = document.getElementById('collectionTableBody');
             
             if (collectionData.items.length === 0) {
                 tbody.innerHTML = `
                     <tr>
                         <td colspan="6" class="empty-state">
                             <div class="empty-icon">📀</div>
                             <div class="empty-title">Collection empty</div>
                         </td>
                     </tr>
                 `;
                 return;
             }
             
             tbody.innerHTML = collectionData.items.map(item => `
                 <tr>
                     <td>
                        <div class="table-image">
                            <img src="${item.image_url || '/api/image/' + item.id}" alt="${item.title}" onerror="this.src='data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHRleHQgeT0iNTAlIiB4PSI1MCUiIGR5PSIuM2VtIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LXNpemU9IjI0Ij7wn5iAPC90ZXh0Pjwvc3ZnPg=='" loading="lazy">
                        </div>
                     </td>
                     <td>
                         <div style="font-weight: 600;">${item.title}</div>
                         <div style="font-size: 0.8rem; color: var(--text-muted);">
                            <a href="${item.url}" target="_blank" style="color: var(--primary); text-decoration: none;">
                                🔗 Source
                            </a>
                        </div>
                     </td>
                     <td>
                        <span class="badge ${item.is_uhd_4k ? 'badge-warning' : 'badge-info'}">
                            ${item.is_uhd_4k ? '4K UHD' : 'Bluray'}
                        </span>
                     </td>
                     <td>€${item.purchase_price.toFixed(2)}</td>
                     <td>${new Date(item.added_at || new Date()).toLocaleDateString()}</td>
                     <td>
                        <button class="btn btn-secondary" onclick="editCollectionItem(${item.id})">✏️</button>
                        <button class="btn btn-danger" onclick="deleteCollectionItem(${item.id})">🗑️</button>
                     </td>
                 </tr>
             `).join('');
        }
        
        // Modals
        function openModal(id) {
            document.getElementById(id).classList.add('active');
        }
        
        function closeModal(id) {
            document.getElementById(id).classList.remove('active');
        }
        
        window.onclick = (e) => {
            if (e.target.classList.contains('modal')) {
                e.target.classList.remove('active');
            }
        };
        
        // Actions
        function openAddWishlistModal() {
            document.getElementById('addWishlistForm').reset();
            openModal('addWishlistModal');
        }
        
        function openEditWishlistModal(id) {
            const item = wishlistData.items.find(i => i.id === id);
            if (!item) return;
            
            document.getElementById('editWishlistId').value = id;
            document.getElementById('editWishlistTitle').value = item.title;
            document.getElementById('editWishlistTitleLocked').checked = item.title_locked;
            document.getElementById('editWishlistPrice').value = item.desired_max_price;
            document.getElementById('editWishlistNotifyPrice').checked = item.notify_on_price_drop;
            document.getElementById('editWishlistNotifyStock').checked = item.notify_on_stock;
            
            openModal('editWishlistModal');
        }

        function openAddCollectionModal() {
            document.getElementById('addCollectionForm').reset();
            openModal('addCollectionModal');
        }
        
        function editCollectionItem(id) {
             const item = collectionData.items.find(i => i.id === id);
             if (!item) return;

             document.getElementById('editCollectionId').value = id;
             document.getElementById('editCollectionTitle').value = item.title;
             document.getElementById('editCollectionPrice').value = item.purchase_price;
             document.getElementById('editCollectionIsUHD').checked = item.is_uhd_4k;
             document.getElementById('editCollectionNotes').value = item.notes || ''; // Assuming notes field exists
             
             openModal('editCollectionModal');
        }
        
        async function updateCollectionItem(event) {
            event.preventDefault();
            const id = document.getElementById('editCollectionId').value;
            const data = {
                title: document.getElementById('editCollectionTitle').value,
                purchase_price: parseFloat(document.getElementById('editCollectionPrice').value),
                is_uhd_4k: document.getElementById('editCollectionIsUHD').checked,
                notes: document.getElementById('editCollectionNotes').value
            };
            
            try {
                 const res = await fetch(`/api/collection/${id}`, {
                     method: 'PUT',
                     headers: { 'Content-Type': 'application/json' },
                     body: JSON.stringify(data)
                 });

                 if (res.ok) {
                     showToast('Item updated', 'success');
                     closeModal('editCollectionModal');
                     loadCollection(collectionData.page);
                 } else {
                     showToast('Failed to update item', 'error');
                 }
            } catch (error) {
                 showToast(`Error: ${error.message}`, 'error');
            }
        }

        async function deleteCollectionItem(id) {
            if (!confirm('Are you sure you want to delete this item?')) return;

            try {
                const res = await fetch(`/api/collection/${id}`, { method: 'DELETE' });
                if (res.ok) {
                    showToast('Item deleted', 'success');
                    loadCollection(collectionData.page);
                } else {
                    showToast('Failed to delete item', 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            }
        }

        async function addWishlistItem(event) {
            event.preventDefault();
            
            const data = {
                url: document.getElementById('wishlistUrl').value,
                desired_max_price: parseFloat(document.getElementById('wishlistPrice').value),
                notify_on_price_drop: document.getElementById('notifyPrice').checked,
                notify_on_stock: document.getElementById('notifyStock').checked
            };
            
            try {
                const res = await fetch('/api/wishlist', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });
                
                if (res.ok) {
                    showToast('Item added to wishlist', 'success');
                    closeModal('addWishlistModal');
                    loadWishlist();
                } else {
                    showToast('Failed to add item', 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            }
        }
        
        async function updateWishlistItem(event) {
            event.preventDefault();

            const id = document.getElementById('editWishlistId').value;
            const data = {
                title: document.getElementById('editWishlistTitle').value,
                title_locked: document.getElementById('editWishlistTitleLocked').checked,
                desired_max_price: parseFloat(document.getElementById('editWishlistPrice').value),
                notify_on_price_drop: document.getElementById('editWishlistNotifyPrice').checked,
                notify_on_stock: document.getElementById('editWishlistNotifyStock').checked
            };

            try {
                const res = await fetch(`/api/wishlist/${id}`, {
                    method: 'PUT',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });

                if (res.ok) {
                    showToast('Item updated', 'success');
                    closeModal('editWishlistModal');
                    loadWishlist(wishlistData.page);
                } else {
                    showToast('Failed to update item', 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            }
        }

        async function deleteWishlistItem(id) {
            if (!confirm('Are you sure you want to delete this item?')) return;

            try {
                const res = await fetch(`/api/wishlist/${id}`, { method: 'DELETE' });
                if (res.ok) {
                    showToast('Item deleted', 'success');
                    loadWishlist(wishlistData.page);
                } else {
                    showToast('Failed to delete item', 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            }
        }

        async function addCollectionItem(event) {
            event.preventDefault();
            // TODO: Implementation
            showToast('Feature coming soon', 'info');
        }
        
        async function scrapeNow() {
            const btn = document.getElementById('scrapeBtn');
            btn.disabled = true;
            btn.innerHTML = '<div class="loading"></div> Scraping...';
            
            try {
                const res = await fetch('/api/action/scrape', { method: 'POST' });
                if (res.ok) {
                    showToast('Scraping started', 'success');
                    // Polling will handle the rest via loadDashboardStats
                } else {
                    showToast('Failed to start scrape', 'error');
                    btn.disabled = false;
                    btn.innerHTML = '<span>🔄</span> Scrape Now';
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
                btn.disabled = false;
                btn.innerHTML = '<span>🔄</span> Scrape Now';
            }
        }

        async function loadSettings() {
            try {
                const res = await fetch('/api/settings');
                const data = await res.json();
                
                document.getElementById('scrapeDelay').value = data.scrape_delay;
                document.getElementById('cacheDir').value = data.cache_dir;
            } catch (error) {
                console.error('Failed to load settings:', error);
            }
        }

        async function saveSettings(event) {
            event.preventDefault();
            
            const data = {
                scrape_delay_seconds: parseInt(document.getElementById('scrapeDelay').value),
                cache_directory: document.getElementById('cacheDir').value
            };
            
            try {
                const res = await fetch('/api/settings', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });
                
                if (res.ok) {
                    showToast('Settings saved', 'success');
                } else {
                    showToast('Failed to save settings', 'error');
                }
            } catch (error) {
                showToast(`Error: ${error.message}`, 'error');
            }
        }

        function toggleTheme() {
            const body = document.body;
            const current = body.getAttribute('data-theme');
            const next = current === 'dark' ? 'light' : 'dark';
            body.setAttribute('data-theme', next);
            
            const btn = document.querySelector('.theme-toggle');
            btn.textContent = next === 'dark' ? '🌙' : '☀️';
        }

        function showToast(message, type = 'info') {
            const container = document.getElementById('toastContainer');
            const toast = document.createElement('div');
            toast.className = `toast toast-${type}`;
            toast.innerHTML = `
                <span class="toast-icon">${type === 'success' ? '✅' : type === 'error' ? '❌' : 'ℹ️'}</span>
                <span>${message}</span>
            `;
            
            container.appendChild(toast);
            
            setTimeout(() => {
                toast.style.animation = 'slideInRight 0.3s ease reverse';
                setTimeout(() => toast.remove(), 300);
            }, 3000);
        }
        
        // Price History Logic
        async function openPriceHistory(id) {
             const item = wishlistData.items.find(i => i.id === id);
             if (!item) return;
             
             document.getElementById('historyTitle').textContent = `Price History: ${item.title}`;
             openModal('priceHistoryModal');
             
             // Load Data
             try {
                const res = await fetch(`/api/wishlist/${id}/history`);
                const history = await res.json();
                renderPriceChart(history, item.desired_max_price);
             } catch(e) {
                console.error("Failed to load history", e);
             }
        }
        
        function renderPriceChart(history, targetPrice) {
            const ctx = document.getElementById('priceHistoryChart').getContext('2d');
            
            if (chartInstance) {
                chartInstance.destroy();
            }
            
            const labels = history.map(h => {
                const dateStr = h.date.replace(' ', 'T'); // Ensure ISO format for safety
                return new Date(dateStr).toLocaleDateString();
            });
            const prices = history.map(h => h.price);
            const targets = history.map(() => targetPrice);
            
            chartInstance = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{
                        label: 'Price',
                        data: prices,
                        borderColor: '#667eea',
                        backgroundColor: 'rgba(102, 126, 234, 0.1)',
                        fill: true,
                        tension: 0.4
                    }, {
                        label: 'Target Price',
                        data: targets,
                        borderColor: '#10b981',
                        borderDash: [5, 5],
                        pointRadius: 0,
                        fill: false
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    scales: {
                        y: {
                            beginAtZero: false,
                            grid: { color: 'rgba(255, 255, 255, 0.1)' },
                            ticks: { color: '#94a3b8' }
                        },
                        x: {
                            grid: { display: false },
                            ticks: { color: '#94a3b8' }
                        }
                    },
                    plugins: {
                        legend: { labels: { color: '#f1f5f9' } }
                    }
                }
            });
        }
    </script>
)HTML";
}

} // namespace bluray::presentation
