/**
 * SEC EDGAR Fraud Analyzer - Web Client Application
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * DISCLAIMER: This project is NOT funded, endorsed, or approved by the
 * U.S. Securities and Exchange Commission (SEC).
 * 
 * Features:
 * - Config.json support for persistent settings
 * - CIK lookup for delisted companies
 * - Improved connection settings UX
 * - Demo mode for testing without server
 */

'use strict';

// Application State
const APP_VERSION = '3.1.3';
let apiBaseUrl = '';
let currentData = null;
let isConnected = false;
let isDemoMode = false;
let isDarkMode = false;
let requestTimeout = 30;
let analysisHistory = [];
let _detectBrowserLocale = false;   // controlled by client.detect_browser_locale in config.pson

// =============================================================================
// Logger - Adjustable logging levels for debugging
// =============================================================================
const LogLevel = {
    DEBUG: 0,
    INFO: 1,
    WARNING: 2,
    ERROR: 3,
    CRITICAL: 4,
    NONE: 5
};

const Logger = {
    level: LogLevel.INFO,
    showTimestamp: true,
    showLevel: true,
    logHistory: [],
    maxHistory: 1000,
    
    // Set log level from string
    setLevel(levelStr) {
        const levels = {
            'debug': LogLevel.DEBUG,
            'info': LogLevel.INFO,
            'warning': LogLevel.WARNING,
            'warn': LogLevel.WARNING,
            'error': LogLevel.ERROR,
            'critical': LogLevel.CRITICAL,
            'none': LogLevel.NONE
        };
        const lower = (levelStr || 'info').toLowerCase();
        this.level = levels[lower] !== undefined ? levels[lower] : LogLevel.INFO;
        this.info(`Log level set to: ${levelStr}`);
    },
    
    // Get current level as string
    getLevelString() {
        const names = ['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL', 'NONE'];
        return names[this.level] || 'UNKNOWN';
    },
    
    // Format log message
    formatMessage(level, ...args) {
        const parts = [];
        
        if (this.showTimestamp) {
            const now = new Date();
            const ts = now.toISOString().replace('T', ' ').substr(0, 23);
            parts.push(`[${ts}]`);
        }
        
        if (this.showLevel) {
            const levelNames = ['DEBUG', 'INFO ', 'WARN ', 'ERROR', 'CRIT '];
            parts.push(`[${levelNames[level]}]`);
        }
        
        return parts.length > 0 ? parts.join(' ') + ' ' : '';
    },
    
    // Store in history
    addToHistory(level, message) {
        const entry = {
            timestamp: new Date().toISOString(),
            level: level,
            message: message
        };
        this.logHistory.push(entry);
        if (this.logHistory.length > this.maxHistory) {
            this.logHistory.shift();
        }
    },
    
    // Log methods
    debug(...args) {
        if (this.level <= LogLevel.DEBUG) {
            const prefix = this.formatMessage(LogLevel.DEBUG);
            console.debug(prefix, ...args);
            this.addToHistory('DEBUG', args.join(' '));
        }
    },
    
    info(...args) {
        if (this.level <= LogLevel.INFO) {
            const prefix = this.formatMessage(LogLevel.INFO);
            console.info(prefix, ...args);
            this.addToHistory('INFO', args.join(' '));
        }
    },
    
    warning(...args) {
        if (this.level <= LogLevel.WARNING) {
            const prefix = this.formatMessage(LogLevel.WARNING);
            console.warn(prefix, ...args);
            this.addToHistory('WARNING', args.join(' '));
        }
    },
    warn(...args) {
        this.warning(...args);
    },
    
    error(...args) {
        if (this.level <= LogLevel.ERROR) {
            const prefix = this.formatMessage(LogLevel.ERROR);
            console.error(prefix, ...args);
            this.addToHistory('ERROR', args.join(' '));
        }
    },
    
    critical(...args) {
        if (this.level <= LogLevel.CRITICAL) {
            const prefix = this.formatMessage(LogLevel.CRITICAL);
            console.error(prefix, '***', ...args, '***');
            this.addToHistory('CRITICAL', args.join(' '));
        }
    },
    
    // Get log history (for debug panel)
    getHistory(maxEntries = 100) {
        return this.logHistory.slice(-maxEntries);
    },
    
    // Clear history
    clearHistory() {
        this.logHistory = [];
    },
    
    // Export history as text
    exportHistory() {
        return this.logHistory.map(e => 
            `${e.timestamp} [${e.level}] ${e.message}`
        ).join('\n');
    }
};

// Make Logger available globally for debugging in console
window.Logger = Logger;
window.LogLevel = LogLevel;

// Initialize Application
document.addEventListener('DOMContentLoaded', () => {
    initializeApp();
});

async function initializeApp() {
    // Load config.pson first — sets _detectBrowserLocale before I18n.init()
    await loadConfigFile();
    loadSettings();

    // Initialise i18n — _detectBrowserLocale is now set from config.pson
    I18n.init(_detectBrowserLocale);
    setupEventListeners();
    setupKeyboardShortcuts();
    updateDarkModeIcon();   // sync icon to restored dark/light preference

    // Sync locale selector to active locale
    const sel = document.getElementById('locale-select');
    if (sel) sel.value = I18n.getLocale();

    // Apply translations to all data-i18n elements
    I18n.applyToDocument();

    // localStorage takes priority over config.pson (user's manual override)
    const storedUrl = localStorage.getItem('apiUrl');
    if (storedUrl) {
        apiBaseUrl = normalizeApiUrl(storedUrl);
        localStorage.setItem('apiUrl', apiBaseUrl);
    }

    await checkServerConnection();
    loadHistory();
}

// Normalize API URL - remove trailing /api/health, /api/, or trailing slashes
function normalizeApiUrl(url) {
    if (!url) return '';
    url = url.trim();
    
    // Remove common endpoint paths that users might accidentally include
    const suffixesToRemove = [
        '/api/health',
        '/api/analyze',
        '/api/filings',
        '/api/',
        '/api'
    ];
    
    for (const suffix of suffixesToRemove) {
        if (url.endsWith(suffix)) {
            url = url.slice(0, -suffix.length);
        }
    }
    
    // Remove trailing slash
    while (url.endsWith('/')) {
        url = url.slice(0, -1);
    }
    
    return url;
}

// Config.json Support
// ---------------------------------------------------------------------------
// PSON parser — mirrors the C++ Pson class
// Lines: key=value  (#comments and blank lines ignored)
// Values may be quoted; inline # comments are stripped
// ---------------------------------------------------------------------------
function parsePson(text) {
    const result = {};
    for (const rawLine of text.split('\n')) {
        let line = rawLine.trim();
        if (!line || line.startsWith('#')) continue;
        const eq = line.indexOf('=');
        if (eq < 0) continue;
        const key = line.slice(0, eq).trim();
        let val = line.slice(eq + 1).trim();
        // Strip inline comment (space + #)
        const ci = val.indexOf(' #');
        if (ci >= 0) val = val.slice(0, ci).trim();
        // Strip surrounding quotes
        if (val.length >= 2 &&
            ((val[0] === '"' && val[val.length-1] === '"') ||
             (val[0] === "'" && val[val.length-1] === "'"))) {
            val = val.slice(1, -1);
        }
        if (key) result[key] = val;
    }
    return result;
}

async function loadConfigFile() {
    try {
        const response = await fetch('config.pson');
        if (response.ok) {
            const text = await response.text();
            const pson = parsePson(text);
            applyConfig(pson);
            Logger.info('Loaded config.pson');
        }
    } catch (e) {
        Logger.debug('No config.pson found, using defaults');
    }
}

function applyConfig(cfg) {
    // client.api_url — primary connection target
    if (cfg['client.api_url']) {
        apiBaseUrl = normalizeApiUrl(cfg['client.api_url']);
    }
    // Legacy key (JSON era)
    if (cfg['apiUrl']) {
        apiBaseUrl = normalizeApiUrl(cfg['apiUrl']);
    }
    if (cfg['client.timeout_seconds']) {
        requestTimeout = parseInt(cfg['client.timeout_seconds'], 10) || 30;
    }
    if (cfg['client.log_level']) {
        Logger.setLevel(cfg['client.log_level']);
    }
    if (cfg['client.dark_mode'] !== undefined) {
        isDarkMode = (cfg['client.dark_mode'] === 'true' || cfg['client.dark_mode'] === true);
        document.body.classList.toggle('dark-mode', isDarkMode);
    }
    if (cfg['client.demo_mode'] !== undefined) {
        isDemoMode = (cfg['client.demo_mode'] === 'true' || cfg['client.demo_mode'] === true);
        updateDemoModeUI();
    }
    // client.detect_browser_locale — if true, I18n.init() will detect the
    // browser's preferred language when no locale is stored in localStorage.
    // If false (default), American English ('en') is always the default.
    if (cfg['client.detect_browser_locale'] !== undefined) {
        _detectBrowserLocale = (cfg['client.detect_browser_locale'] === 'true' ||
                                 cfg['client.detect_browser_locale'] === true);
    }
}

function saveConfigFile() {
    const lines = [
        '# Metis SEC EDGAR Fraud Analyzer - Client Configuration',
        '# Generated by client - ' + new Date().toISOString(),
        '',
        '# Server connection (set to the address where metis-sec-edgar-fraud-analyzer is running)',
        'client.api_url=' + (apiBaseUrl || 'http://localhost:8080'),
        'client.timeout_seconds=' + requestTimeout,
        '',
        '# Client preferences',
        'client.dark_mode=' + isDarkMode,
        'client.demo_mode=' + isDemoMode,
        'client.detect_browser_locale=' + _detectBrowserLocale,
    ];

    const blob = new Blob([lines.join('\n') + '\n'], { type: 'text/plain' });
    const url  = URL.createObjectURL(blob);
    const a    = document.createElement('a');
    a.href     = url;
    a.download = 'config.pson';
    a.click();
    URL.revokeObjectURL(url);

    showAlert(I18n.t('alert.config_saved'), 'success');
}

// Settings Management
function loadSettings() {
    const storedUrl = localStorage.getItem('apiUrl');
    if (storedUrl && !apiBaseUrl) {
        apiBaseUrl = normalizeApiUrl(storedUrl);
    }
    
    const storedTimeout = localStorage.getItem('requestTimeout');
    if (storedTimeout) {
        requestTimeout = parseInt(storedTimeout, 10);
    }
    
    const storedDarkMode = localStorage.getItem('darkMode');
    if (storedDarkMode === 'true') {
        isDarkMode = true;
        document.body.classList.add('dark-mode');
    }
    
    const storedDemoMode = localStorage.getItem('demoMode');
    if (storedDemoMode === 'true') {
        isDemoMode = true;
        updateDemoModeUI();
    }
}

function saveSettings() {
    localStorage.setItem('apiUrl', apiBaseUrl);
    localStorage.setItem('requestTimeout', requestTimeout.toString());
    localStorage.setItem('darkMode', isDarkMode.toString());
    localStorage.setItem('demoMode', isDemoMode.toString());
}

// Event Listeners
function setupEventListeners() {
    // Ticker input - Enter key
    document.getElementById('ticker').addEventListener('keypress', (e) => {
        if (e.key === 'Enter') analyzeCompany();
    });
    
    // CIK input - Enter key
    document.getElementById('cik').addEventListener('keypress', (e) => {
        if (e.key === 'Enter') analyzeCompany();
    });
    
    // Tab navigation
    document.querySelectorAll('.tab').forEach(tab => {
        tab.addEventListener('click', () => switchTab(tab.dataset.tab));
    });
    
    // Close modals on backdrop click
    document.querySelectorAll('.modal').forEach(modal => {
        modal.addEventListener('click', (e) => {
            if (e.target === modal) {
                closeModal(modal.id);
            }
        });
    });
    
    // Menu handling
    document.querySelectorAll('.menu-item').forEach(item => {
        item.addEventListener('mouseenter', () => {
            document.querySelectorAll('.menu-item').forEach(m => m.classList.remove('active'));
            item.classList.add('active');
        });
    });
    
    document.addEventListener('click', (e) => {
        if (!e.target.closest('.menu-item')) {
            document.querySelectorAll('.menu-item').forEach(m => m.classList.remove('active'));
        }
    });
}

// Keyboard Shortcuts
function setupKeyboardShortcuts() {
    document.addEventListener('keydown', (e) => {
        // Check for Ctrl/Cmd key combinations
        if (e.ctrlKey || e.metaKey) {
            switch (e.key.toLowerCase()) {
                case 's':
                    e.preventDefault();
                    exportResults('json');
                    break;
                case 'e':
                    e.preventDefault();
                    exportResults('csv');
                    break;
                case 'h':
                    e.preventDefault();
                    exportResults('html');
                    break;
                case 'p':
                    e.preventDefault();
                    printReport();
                    break;
                case 'd':
                    e.preventDefault();
                    toggleDarkMode();
                    break;
                case 'm':
                    e.preventDefault();
                    toggleDemoMode();
                    break;
            }
        }
        
        // F1 for help, F2 for server info, F3 for logs
        if (e.key === 'F1') {
            e.preventDefault();
            showHelp();
        }
        if (e.key === 'F2') {
            e.preventDefault();
            showServerInfo();
        }
        if (e.key === 'F3') {
            e.preventDefault();
            showLogs();
        }
        
        // Escape to close modals
        if (e.key === 'Escape') {
            document.querySelectorAll('.modal.active').forEach(m => {
                closeModal(m.id);
            });
        }
    });
}

// Connection Management
async function checkServerConnection() {
    Logger.debug('Checking server connection...');
    
    if (isDemoMode) {
        Logger.info('Demo mode active - skipping server connection');
        updateConnectionStatus(true, APP_VERSION + ' (Demo)');
        return;
    }
    
    if (!apiBaseUrl) {
        Logger.warning('No API URL configured');
        updateConnectionStatus(false);
        return;
    }
    
    try {
        Logger.debug(`Connecting to: ${apiBaseUrl}/api/health`);
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 5000);
        
        const response = await fetch(`${apiBaseUrl}/api/health`, {
            signal: controller.signal
        });
        clearTimeout(timeoutId);
        
        if (response.ok) {
            const data = await response.json();
            Logger.info(`Connected to server v${data.version || 'Unknown'}`);
            updateConnectionStatus(true, data.version || 'Unknown');
            isConnected = true;
        } else {
            Logger.error(`Server returned status: ${response.status}`);
            updateConnectionStatus(false);
            isConnected = false;
        }
    } catch (e) {
        Logger.error(`Connection failed: ${e.message}`);
        updateConnectionStatus(false);
        isConnected = false;
    }
}

function updateConnectionStatus(connected, version) {
    const dot = document.getElementById('status-dot');
    const text = document.getElementById('status-text');
    const connStatus = document.getElementById('conn-status');
    const connServer = document.getElementById('conn-server');
    const connVersion = document.getElementById('conn-version');
    
    if (connected) {
        dot.className = 'status-dot connected';
        text.textContent = I18n.t('status.connected');
        if (connStatus) connStatus.textContent = I18n.t('status.connected');
        if (connServer) connServer.textContent = apiBaseUrl || '(same origin)';
        if (connVersion) connVersion.textContent = version || '-';
    } else {
        dot.className = 'status-dot disconnected';
        text.textContent = isDemoMode ? I18n.t('status.demo_mode') : I18n.t('status.not_connected');
        if (connStatus) connStatus.textContent = isDemoMode ? I18n.t('status.demo_mode') : I18n.t('status.not_connected');
        if (connServer) connServer.textContent = apiBaseUrl || '(not configured)';
        if (connVersion) connVersion.textContent = '-';
    }
    
    isConnected = connected;
}

function showConnectionSettings() {
    const modal = document.getElementById('connection-modal');
    const urlInput = document.getElementById('api-url');
    const timeoutInput = document.getElementById('timeout');
    
    urlInput.value = apiBaseUrl;
    timeoutInput.value = requestTimeout;
    
    // Update connection info display
    const connStatus = document.getElementById('conn-status');
    const connServer = document.getElementById('conn-server');
    
    if (connStatus) {
        connStatus.textContent = isConnected ? 'Connected' : (isDemoMode ? I18n.t('status.demo_mode') : I18n.t('status.not_connected'));
    }
    if (connServer) {
        connServer.textContent = apiBaseUrl || '(not configured)';
    }
    
    modal.classList.add('active');
}

function testConnectionFromModal() {
    const urlInput = document.getElementById('api-url');
    let url = normalizeApiUrl(urlInput.value);
    urlInput.value = url; // Update input with normalized value
    
    const tempUrl = apiBaseUrl;
    apiBaseUrl = url;
    
    testConnection().then(() => {
        if (!isConnected) {
            apiBaseUrl = tempUrl;
        }
    });
}

async function testConnection() {
    if (isDemoMode) {
        showAlert('Demo mode is active. Disable demo mode to test real connection.', 'info');
        return;
    }
    
    if (!apiBaseUrl) {
        showAlert(I18n.t('alert.enter_url'), 'warning');
        return;
    }
    
    showAlert(I18n.t('alert.testing'), 'info');
    
    try {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 5000);
        
        const response = await fetch(`${apiBaseUrl}/api/health`, {
            signal: controller.signal
        });
        clearTimeout(timeoutId);
        
        if (response.ok) {
            const data = await response.json();
            updateConnectionStatus(true, data.version || 'Unknown');
            showAlert('Connection successful! Server version: ' + (data.version || 'Unknown'), 'success');
        } else {
            updateConnectionStatus(false);
            showAlert('Server responded with error: ' + response.status, 'error');
        }
    } catch (e) {
        updateConnectionStatus(false);
        if (e.name === 'AbortError') {
            showAlert('Connection timed out. Check that the server is running.', 'error');
        } else {
            showAlert('Connection failed: ' + e.message, 'error');
        }
    }
}

function saveConnectionSettings() {
    const urlInput = document.getElementById('api-url');
    const timeoutInput = document.getElementById('timeout');
    
    apiBaseUrl = normalizeApiUrl(urlInput.value);
    requestTimeout = parseInt(timeoutInput.value, 10) || 30;
    
    saveSettings();
    closeModal('connection-modal');
    checkServerConnection();
}

// CIK Lookup
function toggleCIKInput() {
    const useCik = document.getElementById('use-cik').checked;
    const tickerGroup = document.querySelector('.ticker-group');
    const cikGroup = document.querySelector('.cik-group');
    
    if (useCik) {
        tickerGroup.style.display = 'none';
        cikGroup.style.display = 'block';
    } else {
        tickerGroup.style.display = 'block';
        cikGroup.style.display = 'none';
    }
}

function showCIKLookup() {
    document.getElementById('cik-modal').classList.add('active');
}

async function searchCIK() {
    const searchTerm = document.getElementById('cik-search').value.trim();
    if (!searchTerm) {
        showAlert(I18n.t('alert.enter_search'), 'warning');
        return;
    }
    
    if (isDemoMode) {
        // Show demo results
        const demoResults = {
            'enron': { name: 'Enron Corp', cik: '0001024401' },
            'worldcom': { name: 'WorldCom Inc', cik: '0000723527' },
            'lehman': { name: 'Lehman Brothers Holdings Inc', cik: '0000806085' }
        };
        
        const term = searchTerm.toLowerCase();
        for (const [key, value] of Object.entries(demoResults)) {
            if (term.includes(key)) {
                showAlert(`Found: ${value.name} - CIK: ${value.cik}`, 'success');
                document.getElementById('cik').value = value.cik;
                document.getElementById('use-cik').checked = true;
                toggleCIKInput();
                closeModal('cik-modal');
                return;
            }
        }
        showAlert(I18n.t('alert.no_demo_company'), 'warning');
        return;
    }
    
    if (!isConnected) {
        showAlert(I18n.t('alert.not_connected'), 'error');
        return;
    }
    
    try {
        const response = await fetch(`${apiBaseUrl}/api/search?q=${encodeURIComponent(searchTerm)}`);
        if (response.ok) {
            const data = await response.json();
            if (data.results && data.results.length > 0) {
                const result = data.results[0];
                showAlert(`Found: ${result.name} - CIK: ${result.cik}`, 'success');
                document.getElementById('cik').value = result.cik;
                document.getElementById('use-cik').checked = true;
                toggleCIKInput();
                closeModal('cik-modal');
            } else {
                showAlert('No matching company found.', 'warning');
            }
        } else {
            showAlert(I18n.t('alert.cik_failed'), 'error');
        }
    } catch (e) {
        showAlert('CIK search error: ' + e.message, 'error');
    }
}

// Analysis Functions
async function analyzeCompany() {
    const useCik = document.getElementById('use-cik').checked;
    let identifier;
    
    if (useCik) {
        identifier = document.getElementById('cik').value.trim();
        if (!identifier) {
            showAlert('Please enter a CIK number.', 'warning');
            return;
        }
        Logger.info(`Starting analysis for CIK: ${identifier}`);
    } else {
        identifier = document.getElementById('ticker').value.trim().toUpperCase();
        if (!identifier) {
            showAlert('Please enter a ticker symbol.', 'warning');
            return;
        }
        Logger.info(`Starting analysis for ticker: ${identifier}`);
    }
    
    const scope = document.getElementById('scope').value;
    const include10K = document.getElementById('include-10k').checked;
    const include10Q = document.getElementById('include-10q').checked;
    const includeAmendments = document.getElementById('include-amendments').checked;
    const includeRaw = document.getElementById('include-raw').checked;
    
    Logger.debug(`Analysis options: scope=${scope}, 10K=${include10K}, 10Q=${include10Q}, amendments=${includeAmendments}`);
    
    // Show loading
    document.getElementById('loading').classList.remove('hidden');
    document.getElementById('results').classList.add('hidden');
    
    if (isDemoMode) {
        Logger.debug('Using demo mode - generating fake data');
        // Generate demo data
        setTimeout(() => {
            currentData = generateDemoData(identifier);
            displayResults(currentData);
            addToHistory(identifier, currentData);
            document.getElementById('loading').classList.add('hidden');
            Logger.info(`Demo analysis complete for ${identifier}`);
        }, 1500);
        return;
    }
    
    if (!isConnected) {
        Logger.error('Analysis failed - not connected to server');
        document.getElementById('loading').classList.add('hidden');
        showAlert('Not connected to server. Enable Demo Mode or configure connection.', 'error');
        return;
    }
    
    try {
        const params = new URLSearchParams({
            years: scope,
            include_10k: include10K,
            include_10q: include10Q,
            include_amendments: includeAmendments,
            include_raw: includeRaw
        });
        
        if (useCik) {
            params.append('cik', identifier);
        } else {
            params.append('ticker', identifier);
        }
        
        const url = `${apiBaseUrl}/api/analyze?${params}`;
        Logger.debug(`Fetching: ${url}`);
        
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), requestTimeout * 1000);
        
        const startTime = performance.now();
        const response = await fetch(url, {
            signal: controller.signal
        });
        clearTimeout(timeoutId);
        const elapsed = ((performance.now() - startTime) / 1000).toFixed(2);
        
        if (response.ok) {
            const data = await response.json();
            currentData = data;
            displayResults(data);
            addToHistory(identifier, data);
            Logger.info(`Analysis complete for ${identifier} in ${elapsed}s`);
        } else {
            const errorData = await response.json().catch(() => ({}));
            Logger.error(`Analysis failed: ${response.status} - ${errorData.error || 'Unknown error'}`);
            showAlert(errorData.error || 'Analysis failed: ' + response.status, 'error');
        }
    } catch (e) {
        if (e.name === 'AbortError') {
            showAlert('Request timed out. Try increasing the timeout in settings.', 'error');
        } else {
            showAlert('Unable to find company with ticker: ' + identifier, 'error');
        }
    } finally {
        document.getElementById('loading').classList.add('hidden');
    }
}

async function listFilings() {
    const ticker = document.getElementById('ticker').value.trim().toUpperCase();
    if (!ticker) {
        showAlert('Please enter a ticker symbol.', 'warning');
        return;
    }
    
    if (isDemoMode) {
        showAlert('Filing list not available in demo mode.', 'info');
        return;
    }
    
    if (!isConnected) {
        showAlert(I18n.t('alert.not_connected_short'), 'error');
        return;
    }
    
    try {
        const response = await fetch(`${apiBaseUrl}/api/filings?ticker=${ticker}`);
        if (response.ok) {
            const data = await response.json();
            displayFilingsList(data);
        } else {
            showAlert('Failed to fetch filings list.', 'error');
        }
    } catch (e) {
        showAlert('Error fetching filings: ' + e.message, 'error');
    }
}

// Display Functions
function displayResults(data) {
    document.getElementById('results').classList.remove('hidden');
    
    // Company header
    const header = document.getElementById('company-header');
    header.innerHTML = `
        <div class="company-name">${escapeHtml(data.company?.name || data.ticker || 'Unknown')}</div>
        <div class="company-info">
            ${data.company?.ticker ? 'Ticker: ' + escapeHtml(data.company.ticker) : ''}
            ${data.company?.cik ? ' | CIK: ' + escapeHtml(data.company.cik) : ''}
            ${data.company?.sic ? ' | SIC: ' + escapeHtml(data.company.sic) : ''}
            | Filings Analyzed: ${data.filings_analyzed || 0}
        </div>
    `;
    
    // Overview tab
    displayOverview(data);
    
    // Models tab
    displayModels(data);
    
    // Filings tab
    displayFilings(data);
    
    // Trends tab
    displayTrends(data);
    
    // Red flags tab
    displayRedFlags(data);
    
    // Switch to overview tab
    switchTab('overview');
}

function displayOverview(data) {
    const container = document.getElementById('tab-overview');
    const risk = data.overall_risk || data.composite_risk || {};
    const riskLevel = risk.level || 'Unknown';
    const riskScore = risk.score || 0;
    
    container.innerHTML = `
        <div class="card-grid">
            <div class="card score-card">
                <div class="score-value ${getRiskClass(riskLevel)}">${(riskScore * 100).toFixed(0)}%</div>
                <div class="score-label">Overall Risk Score</div>
                <span class="score-indicator ${getBgClass(riskLevel)}">${riskLevel}</span>
            </div>
            <div class="card score-card">
                <div class="score-value">${data.filings_analyzed || 0}</div>
                <div class="score-label">Filings Analyzed</div>
            </div>
            <div class="card score-card">
                <div class="score-value ${getRiskClass(data.red_flags?.length > 3 ? 'HIGH' : 'LOW')}">${data.red_flags?.length || 0}</div>
                <div class="score-label">Red Flags Detected</div>
            </div>
        </div>
        
        <div class="card">
            <div class="card-title">Risk Summary</div>
            <p>${risk.summary || 'Analysis complete. Review individual model scores and red flags for details.'}</p>
        </div>
        
        ${data.recommendation ? `
        <div class="card">
            <div class="card-title">Recommendation</div>
            <p>${escapeHtml(data.recommendation)}</p>
        </div>
        ` : ''}
    `;
}

function displayModels(data) {
    const container = document.getElementById('tab-models');
    const models = data.models || {};
    
    let html = '<div class="card-grid">';
    
    // Beneish M-Score
    if (models.beneish) {
        const b = models.beneish;
        html += `
            <div class="card score-card">
                <div class="score-value ${b.m_score > -2.22 ? 'risk-high' : 'risk-low'}">${b.m_score?.toFixed(2) || 'N/A'}</div>
                <div class="score-label">Beneish M-Score</div>
                <span class="score-indicator ${b.m_score > -2.22 ? 'bg-high' : 'bg-low'}">
                    ${b.m_score > -2.22 ? 'Likely Manipulator' : 'Unlikely Manipulator'}
                </span>
                <p style="font-size:12px;color:var(--gray);margin-top:8px;">Threshold: -2.22</p>
            </div>
        `;
    }
    
    // Altman Z-Score
    if (models.altman) {
        const a = models.altman;
        const zone = a.z_score > 2.99 ? 'Safe' : (a.z_score < 1.81 ? 'Distress' : 'Gray');
        html += `
            <div class="card score-card">
                <div class="score-value ${a.z_score < 1.81 ? 'risk-high' : (a.z_score > 2.99 ? 'risk-low' : 'risk-moderate')}">${a.z_score?.toFixed(2) || 'N/A'}</div>
                <div class="score-label">Altman Z-Score</div>
                <span class="score-indicator ${a.z_score < 1.81 ? 'bg-high' : (a.z_score > 2.99 ? 'bg-low' : 'bg-moderate')}">${zone} Zone</span>
            </div>
        `;
    }
    
    // Piotroski F-Score
    if (models.piotroski) {
        const p = models.piotroski;
        html += `
            <div class="card score-card">
                <div class="score-value ${p.f_score >= 7 ? 'risk-low' : (p.f_score <= 3 ? 'risk-high' : 'risk-moderate')}">${p.f_score || 0}</div>
                <div class="score-label">Piotroski F-Score</div>
                <span class="score-indicator ${p.f_score >= 7 ? 'bg-low' : (p.f_score <= 3 ? 'bg-high' : 'bg-moderate')}">
                    ${p.f_score >= 7 ? 'Strong' : (p.f_score <= 3 ? 'Weak' : 'Moderate')}
                </span>
                <p style="font-size:12px;color:var(--gray);margin-top:8px;">Scale: 0-9</p>
            </div>
        `;
    }
    
    // Fraud Triangle
    if (models.fraud_triangle) {
        const ft = models.fraud_triangle;
        html += `
            <div class="card score-card">
                <div class="score-value ${ft.risk_score > 0.6 ? 'risk-high' : (ft.risk_score > 0.3 ? 'risk-moderate' : 'risk-low')}">${(ft.risk_score * 100).toFixed(0)}%</div>
                <div class="score-label">Fraud Triangle Risk</div>
                <span class="score-indicator ${ft.risk_score > 0.6 ? 'bg-high' : (ft.risk_score > 0.3 ? 'bg-moderate' : 'bg-low')}">
                    ${ft.risk_level || 'Unknown'}
                </span>
            </div>
        `;
    }
    
    // Benford's Law
    if (models.benford) {
        const bf = models.benford;
        html += `
            <div class="card score-card">
                <div class="score-value ${bf.suspicious ? 'risk-high' : 'risk-low'}">${bf.deviation?.toFixed(2) || 'N/A'}%</div>
                <div class="score-label">Benford's Law Deviation</div>
                <span class="score-indicator ${bf.suspicious ? 'bg-high' : 'bg-low'}">
                    ${bf.suspicious ? 'Anomaly Detected' : 'Normal'}
                </span>
            </div>
        `;
    }
    
    html += '</div>';
    
    container.innerHTML = html;
}

function displayFilings(data) {
    const container = document.getElementById('tab-filings');
    const filings = data.filings || [];
    
    if (filings.length === 0) {
        container.innerHTML = '<p>No filing data available.</p>';
        return;
    }
    
    let html = `
        <table class="data-table">
            <thead>
                <tr>
                    <th>Filing</th>
                    <th>Date</th>
                    <th>Type</th>
                    <th>Revenue</th>
                    <th>Net Income</th>
                    <th>Risk</th>
                </tr>
            </thead>
            <tbody>
    `;
    
    for (const filing of filings) {
        html += `
            <tr>
                <td>${escapeHtml(filing.accession || '-')}</td>
                <td>${escapeHtml(filing.filed_date || '-')}</td>
                <td>${escapeHtml(filing.form_type || '-')}</td>
                <td>${formatCurrency(filing.revenue)}</td>
                <td>${formatCurrency(filing.net_income)}</td>
                <td><span class="score-indicator ${getBgClass(filing.risk_level || 'LOW')}">${filing.risk_level || '-'}</span></td>
            </tr>
        `;
    }
    
    html += '</tbody></table>';
    container.innerHTML = html;
}

function displayTrends(data) {
    const container = document.getElementById('tab-trends');
    const trends = data.trends || {};
    
    let html = '<div class="card-grid">';
    
    const trendItems = [
        { label: 'Revenue', value: trends.revenue_trend },
        { label: 'Net Income', value: trends.income_trend },
        { label: 'Cash Flow', value: trends.cash_flow_trend },
        { label: 'Debt Ratio', value: trends.debt_trend }
    ];
    
    for (const item of trendItems) {
        const trendClass = item.value === 'IMPROVING' ? 'risk-low' : 
                          (item.value === 'DECLINING' ? 'risk-high' : 'risk-moderate');
        html += `
            <div class="card score-card">
                <div class="score-value ${trendClass}">${getTrendArrow(item.value)}</div>
                <div class="score-label">${item.label}</div>
                <span class="score-indicator">${item.value || 'N/A'}</span>
            </div>
        `;
    }
    
    html += '</div>';
    container.innerHTML = html;
}

function displayRedFlags(data) {
    const container = document.getElementById('tab-redflags');
    const redFlags = data.red_flags || [];
    
    if (redFlags.length === 0) {
        container.innerHTML = `
            <div class="card">
                <p style="text-align:center;color:var(--success);">
                    No significant red flags detected.
                </p>
            </div>
        `;
        return;
    }
    
    let html = '';
    for (const flag of redFlags) {
        html += `
            <div class="red-flag">
                <div class="red-flag-icon">!</div>
                <div class="red-flag-content">
                    <div class="red-flag-title">${escapeHtml(flag.title || flag.type || 'Warning')}</div>
                    <div class="red-flag-description">${escapeHtml(flag.description || flag.message || '')}</div>
                </div>
            </div>
        `;
    }
    
    container.innerHTML = html;
}

function displayFilingsList(data) {
    // Could display in a modal or new tab
    Logger.debug('Filings received:', data);
    showAlert(`Found ${data.filings?.length || 0} filings.`, 'success');
}

// Tab Management
function switchTab(tabName) {
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
    
    document.querySelector(`.tab[data-tab="${tabName}"]`)?.classList.add('active');
    document.getElementById(`tab-${tabName}`)?.classList.add('active');
}

// Mode Toggles
// SVG paths for dark/light mode icons
const ICON_MOON = 'M9 2c-1.05 0-2.05.16-3 .46 4.06 1.27 7 5.06 7 9.54 0 4.48-2.94 8.27-7 9.54.95.3 1.95.46 3 .46 5.52 0 10-4.48 10-10S14.52 2 9 2z';
const ICON_SUN  = 'M6.76 4.84l-1.8-1.79-1.41 1.41 1.79 1.79 1.42-1.41zM4 10.5H1v2h3v-2zm9-9.95h-2V3.5h2V.55zm7.45 3.91l-1.41-1.41-1.79 1.79 1.41 1.41 1.79-1.79zm-3.21 13.7l1.79 1.8 1.41-1.41-1.8-1.79-1.4 1.4zM20 10.5v2h3v-2h-3zm-8-5c-3.31 0-6 2.69-6 6s2.69 6 6 6 6-2.69 6-6-2.69-6-6-6zm-1 16.95h2V19.5h-2v2.95zm-7.45-3.91l1.41 1.41 1.79-1.8-1.41-1.41-1.79 1.8z';

function updateDarkModeIcon() {
    const icon = document.getElementById('dark-mode-icon');
    const btn  = document.getElementById('dark-mode-btn');
    if (!icon) return;
    // In dark mode show sun (switch to light); in light mode show moon (switch to dark)
    icon.querySelector('path').setAttribute('d', isDarkMode ? ICON_SUN : ICON_MOON);
    if (btn) btn.title = I18n.t('toolbar.dark_mode');
}

// ---------------------------------------------------------------------------
// Locale / i18n
// ---------------------------------------------------------------------------
function setLocale(locale) {
    I18n.setLocale(locale);
    // Sync the selector in case setLocale was called programmatically
    const sel = document.getElementById('locale-select');
    if (sel) sel.value = I18n.getLocale();
    // Re-apply dark mode icon (tooltip changes with locale)
    updateDarkModeIcon();
}

function toggleDarkMode() {
    isDarkMode = !isDarkMode;
    document.body.classList.toggle('dark-mode', isDarkMode);
    updateDarkModeIcon();
    saveSettings();
}

function toggleDemoMode() {
    isDemoMode = !isDemoMode;
    updateDemoModeUI();
    saveSettings();
    
    if (isDemoMode) {
        updateConnectionStatus(true, APP_VERSION + ' (Demo)');
        showAlert(I18n.t('alert.demo_enabled'), 'info');
    } else {
        checkServerConnection();
        showAlert(I18n.t('alert.demo_disabled'), 'info');
    }
}

function updateDemoModeUI() {
    const btn = document.getElementById('demo-mode-btn');
    if (btn) {
        btn.classList.toggle('active', isDemoMode);
    }
}

// Export Functions
// Top-level export: prefer server route, fall back to client-side
function exportResults(format) {
    exportViaServer(format);
}

function exportResultsClientSide(format) {
    if (!currentData) {
        showAlert('No data to export. Run an analysis first.', 'warning');
        return;
    }
    
    let content, filename, type;
    
    switch (format) {
        case 'json':
            content = JSON.stringify(currentData, null, 2);
            filename = `fraud-analysis-${currentData.ticker || 'export'}.json`;
            type = 'application/json';
            break;
            
        case 'csv':
            content = convertToCSV(currentData);
            filename = `fraud-analysis-${currentData.ticker || 'export'}.csv`;
            type = 'text/csv';
            break;
            
        case 'html':
            content = generateHTMLReport(currentData);
            filename = `fraud-analysis-${currentData.ticker || 'export'}.html`;
            type = 'text/html';
            break;
            
        default:
            return;
    }
    
    downloadFile(content, filename, type);
    showAlert(`Exported to ${filename}`, 'success');
}

function convertToCSV(data) {
    const rows = [['Metric', 'Value']];
    
    rows.push(['Ticker', data.ticker || '']);
    rows.push(['Company', data.company?.name || '']);
    rows.push(['Filings Analyzed', data.filings_analyzed || 0]);
    rows.push(['Overall Risk Score', data.overall_risk?.score || '']);
    rows.push(['Risk Level', data.overall_risk?.level || '']);
    
    if (data.models?.beneish) {
        rows.push(['Beneish M-Score', data.models.beneish.m_score || '']);
    }
    if (data.models?.altman) {
        rows.push(['Altman Z-Score', data.models.altman.z_score || '']);
    }
    if (data.models?.piotroski) {
        rows.push(['Piotroski F-Score', data.models.piotroski.f_score || '']);
    }
    
    rows.push(['Red Flags', data.red_flags?.length || 0]);
    
    return rows.map(r => r.map(c => `"${c}"`).join(',')).join('\n');
}

function generateHTMLReport(data) {
    return `<!DOCTYPE html>
<html>
<head>
    <title>Fraud Analysis Report - ${escapeHtml(data.ticker || 'Report')}</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }
        h1 { color: #2563eb; }
        .risk-high { color: #dc2626; }
        .risk-low { color: #059669; }
        table { width: 100%; border-collapse: collapse; margin: 20px 0; }
        th, td { padding: 10px; border: 1px solid #ddd; text-align: left; }
        th { background: #f3f4f6; }
    </style>
</head>
<body>
    <h1>SEC EDGAR Fraud Analysis Report</h1>
    <p><strong>Company:</strong> ${escapeHtml(data.company?.name || data.ticker || 'Unknown')}</p>
    <p><strong>Generated:</strong> ${new Date().toLocaleString()}</p>
    <p><strong>Version:</strong> ${APP_VERSION}</p>
    
    <h2>Risk Summary</h2>
    <p><strong>Overall Risk:</strong> <span class="${getRiskClass(data.overall_risk?.level)}">${data.overall_risk?.level || 'Unknown'}</span></p>
    <p><strong>Score:</strong> ${((data.overall_risk?.score || 0) * 100).toFixed(0)}%</p>
    
    <h2>Model Scores</h2>
    <table>
        <tr><th>Model</th><th>Score</th><th>Interpretation</th></tr>
        ${data.models?.beneish ? `<tr><td>Beneish M-Score</td><td>${data.models.beneish.m_score?.toFixed(2)}</td><td>${data.models.beneish.m_score > -2.22 ? 'Likely Manipulator' : 'Unlikely Manipulator'}</td></tr>` : ''}
        ${data.models?.altman ? `<tr><td>Altman Z-Score</td><td>${data.models.altman.z_score?.toFixed(2)}</td><td>${data.models.altman.z_score > 2.99 ? 'Safe' : (data.models.altman.z_score < 1.81 ? 'Distress' : 'Gray')}</td></tr>` : ''}
        ${data.models?.piotroski ? `<tr><td>Piotroski F-Score</td><td>${data.models.piotroski.f_score}</td><td>${data.models.piotroski.f_score >= 7 ? 'Strong' : (data.models.piotroski.f_score <= 3 ? 'Weak' : 'Moderate')}</td></tr>` : ''}
    </table>
    
    <h2>Red Flags (${data.red_flags?.length || 0})</h2>
    <ul>
        ${(data.red_flags || []).map(f => `<li><strong>${escapeHtml(f.title || f.type)}</strong>: ${escapeHtml(f.description || f.message)}</li>`).join('')}
    </ul>
    
    <footer style="margin-top:40px;font-size:12px;color:#666;">
        <p>Generated by SEC EDGAR Fraud Analyzer v${APP_VERSION}</p>
        <p>Author: Bennie Shearer (Retired) | For educational purposes only</p>
    </footer>
</body>
</html>`;
}

function printReport() {
    window.print();
}

// Exit Application — closes browser tab and optionally shuts down the server
function exitApplication() {
    const msg = isConnected
        ? I18n.t('confirm.exit_connected')
        : I18n.t('confirm.exit_disconnected');

    if (!confirm(msg)) return;

    if (isConnected && !isDemoMode) {
        // Signal the server to stop, then close the tab
        fetch(`${apiBaseUrl}/api/shutdown`, { method: 'POST' })
            .catch(() => {}) // ignore — server may already be gone
            .finally(() => {
                window.close();
                setTimeout(() => {
                    document.body.innerHTML =
                        '<div style="display:flex;justify-content:center;align-items:center;height:100vh;font-family:Arial,sans-serif;">' +
                        '<h2>' + I18n.t('close.shutdown') + '</h2></div>';
                }, 200);
            });
    } else {
        window.close();
        setTimeout(() => {
            document.body.innerHTML =
                '<div style="display:flex;justify-content:center;align-items:center;height:100vh;font-family:Arial,sans-serif;">' +
                '<h2>' + I18n.t('close.tab') + '</h2></div>';
        }, 100);
    }
}

// History Management
function addToHistory(identifier, data) {
    const entry = {
        identifier: identifier,
        company: data.company?.name || identifier,
        timestamp: new Date().toISOString(),
        riskLevel: data.overall_risk?.level || 'Unknown'
    };
    
    analysisHistory.unshift(entry);
    if (analysisHistory.length > 50) {
        analysisHistory.pop();
    }
    
    localStorage.setItem('analysisHistory', JSON.stringify(analysisHistory));
}

function loadHistory() {
    const stored = localStorage.getItem('analysisHistory');
    if (stored) {
        try {
            analysisHistory = JSON.parse(stored);
        } catch (e) {
            analysisHistory = [];
        }
    }
}

function showHistory() {
    const container = document.getElementById('history-list');
    
    if (analysisHistory.length === 0) {
        container.innerHTML = '<p style="text-align:center;color:var(--gray);">No analysis history.</p>';
    } else {
        container.innerHTML = analysisHistory.map(entry => `
            <div class="history-item" onclick="rerunAnalysis('${escapeHtml(entry.identifier)}')">
                <div class="history-item-info">
                    <div class="history-item-ticker">${escapeHtml(entry.company || entry.identifier)}</div>
                    <div class="history-item-date">${new Date(entry.timestamp).toLocaleString()}</div>
                </div>
                <span class="score-indicator ${getBgClass(entry.riskLevel)}">${entry.riskLevel}</span>
            </div>
        `).join('');
    }
    
    document.getElementById('history-modal').classList.add('active');
}

function clearHistory() {
    analysisHistory = [];
    localStorage.removeItem('analysisHistory');
    showHistory();
    showAlert('History cleared.', 'success');
}

function rerunAnalysis(identifier) {
    closeModal('history-modal');
    
    // Check if it's a CIK (starts with 0)
    if (identifier.startsWith('0') && identifier.length === 10) {
        document.getElementById('cik').value = identifier;
        document.getElementById('use-cik').checked = true;
        toggleCIKInput();
    } else {
        document.getElementById('ticker').value = identifier;
        document.getElementById('use-cik').checked = false;
        toggleCIKInput();
    }
    
    analyzeCompany();
}

// Batch Analysis
function showBatchAnalysis() {
    document.getElementById('batch-modal').classList.add('active');
}

async function runBatchAnalysis() {
    const textarea = document.getElementById('batch-tickers');
    const scope = document.getElementById('batch-scope').value;
    
    const tickers = textarea.value
        .split(/[,\n]/)
        .map(t => t.trim().toUpperCase())
        .filter(t => t.length > 0);
    
    if (tickers.length === 0) {
        showAlert('Please enter at least one ticker symbol.', 'warning');
        return;
    }
    
    closeModal('batch-modal');
    showAlert(`Starting batch analysis of ${tickers.length} companies...`, 'info');
    
    // For demo, just analyze the first one
    if (isDemoMode) {
        document.getElementById('ticker').value = tickers[0];
        analyzeCompany();
        return;
    }
    
    // Real batch analysis would need server support
    for (const ticker of tickers) {
        document.getElementById('ticker').value = ticker;
        await analyzeCompany();
        await new Promise(r => setTimeout(r, 1000)); // Rate limiting
    }
}

// Cache Management
function clearCache() {
    if (!isConnected && !isDemoMode) {
        showAlert(I18n.t('alert.not_connected_short'), 'error');
        return;
    }
    
    if (isDemoMode) {
        showAlert('Cache cleared (demo mode).', 'success');
        return;
    }
    
    fetch(`${apiBaseUrl}/api/cache/clear`, { method: 'POST' })
        .then(r => {
            if (r.ok) {
                showAlert('Server cache cleared.', 'success');
                fetchCacheStats(); // refresh stats in server info panel if open
            } else {
                showAlert('Failed to clear cache.', 'error');
            }
        })
        .catch(e => showAlert('Error: ' + e.message, 'error'));
}

// Server Info - aggregates /api/version, /api/models, /api/weights,
//               /api/cache/stats, /api/gpu/status, /api/k8s/status
async function showServerInfo() {
    if (!isConnected && !isDemoMode) {
        showAlert(I18n.t('alert.not_connected_short'), 'error');
        return;
    }

    const modal = document.getElementById('server-info-modal');
    const body  = document.getElementById('server-info-body');
    body.innerHTML = '<p style="text-align:center;padding:20px;">Loading...</p>';
    modal.classList.add('active');

    if (isDemoMode) {
        body.innerHTML = buildServerInfoHTML({
            version:  { version: APP_VERSION, app_name: 'Metis SEC EDGAR Fraud Analyzer (Demo)',
                        copyright: 'Copyright (c) 2026 Bennie Shearer (Retired)',
                        license: 'MIT License', disclaimer: 'NOT funded, endorsed, or approved by the U.S. SEC' },
            weights:  { beneish:0.30, altman:0.25, piotroski:0.15, fraud_triangle:0.15, benford:0.05, red_flags:0.10 },
            cache:    { entries: 0, ttl_seconds: 3600 },
            models:   { models: [
                { id:'beneish',        name:'Beneish M-Score',   description:'Earnings manipulation detection' },
                { id:'altman',         name:'Altman Z-Score',    description:'Bankruptcy risk prediction' },
                { id:'piotroski',      name:'Piotroski F-Score', description:'Financial strength (0-9)' },
                { id:'fraud_triangle', name:'Fraud Triangle',    description:'Pressure / Opportunity / Rationalization' },
                { id:'benford',        name:"Benford's Law",     description:'Digit distribution anomaly' }
            ]},
            gpu:      { enabled: false, device: 'auto', status: 'disabled', note: 'Reserved for future C++20 implementation' },
            k8s:      { enabled: false, namespace: 'default', status: 'disabled', note: 'Reserved for future C++20 implementation' }
        });
        return;
    }

    try {
        const [ver, wgt, cst, mdl, gpu, k8s] = await Promise.all([
            fetch(`${apiBaseUrl}/api/version`).then(r => r.json()).catch(() => null),
            fetch(`${apiBaseUrl}/api/weights`).then(r => r.json()).catch(() => null),
            fetch(`${apiBaseUrl}/api/cache/stats`).then(r => r.json()).catch(() => null),
            fetch(`${apiBaseUrl}/api/models`).then(r => r.json()).catch(() => null),
            fetch(`${apiBaseUrl}/api/gpu/status`).then(r => r.json()).catch(() => null),
            fetch(`${apiBaseUrl}/api/k8s/status`).then(r => r.json()).catch(() => null)
        ]);
        body.innerHTML = buildServerInfoHTML({ version: ver, weights: wgt, cache: cst, models: mdl, gpu, k8s });
    } catch (e) {
        body.innerHTML = `<p style="color:var(--danger);">Error loading server info: ${escapeHtml(e.message)}</p>`;
    }
}

async function fetchCacheStats() {
    if (!isConnected || isDemoMode) return;
    try {
        const data = await fetch(`${apiBaseUrl}/api/cache/stats`).then(r => r.json());
        const el = document.getElementById('server-info-cache-entries');
        if (el) el.textContent = data.entries ?? '-';
    } catch (_) {}
}

function buildServerInfoHTML(d) {
    const v   = d.version  || {};
    const w   = d.weights  || {};
    const c   = d.cache    || {};
    const m   = d.models   || {};
    const g   = d.gpu      || {};
    const k   = d.k8s      || {};
    const models = m.models || [];

    const wRow = (label, val) => `<tr><td>${label}</td><td>${typeof val === 'number' ? (val*100).toFixed(0)+'%' : escapeHtml(String(val ?? '-'))}</td></tr>`;
    const pill = (ok, yes, no) => `<span class="score-indicator ${ok ? 'bg-low' : 'bg-moderate'}">${ok ? yes : no}</span>`;

    return `
    <div style="display:grid;gap:16px;">
      <div class="card">
        <div class="card-title">Application</div>
        <table class="data-table">
          <tr><td>Name</td><td>${escapeHtml(v.app_name || '-')}</td></tr>
          <tr><td>Version</td><td>${escapeHtml(v.version || '-')}</td></tr>
          <tr><td>License</td><td>${escapeHtml(v.license || '-')}</td></tr>
          <tr><td>Copyright</td><td>${escapeHtml(v.copyright || '-')}</td></tr>
          <tr><td>Disclaimer</td><td style="font-size:11px;">${escapeHtml(v.disclaimer || '-')}</td></tr>
        </table>
      </div>
      <div class="card">
        <div class="card-title">Model Weights</div>
        <table class="data-table">
          ${wRow('Beneish M-Score', w.beneish)}
          ${wRow('Altman Z-Score', w.altman)}
          ${wRow('Piotroski F-Score', w.piotroski)}
          ${wRow('Fraud Triangle', w.fraud_triangle)}
          ${wRow("Benford's Law", w.benford)}
          ${wRow('Red Flags', w.red_flags)}
        </table>
      </div>
      <div class="card">
        <div class="card-title">Available Models</div>
        <table class="data-table">
          ${models.map(mo => `<tr><td><strong>${escapeHtml(mo.name)}</strong></td><td style="font-size:12px;">${escapeHtml(mo.description)}</td></tr>`).join('')}
        </table>
      </div>
      <div class="card">
        <div class="card-title">Cache</div>
        <table class="data-table">
          <tr><td>Entries</td><td id="server-info-cache-entries">${c.entries ?? '-'}</td></tr>
          <tr><td>TTL</td><td>${c.ttl_seconds != null ? c.ttl_seconds + 's' : '-'}</td></tr>
        </table>
      </div>
      <div class="card">
        <div class="card-title">GPU Backend</div>
        <table class="data-table">
          <tr><td>Status</td><td>${pill(g.enabled, 'Enabled', 'Disabled')}</td></tr>
          <tr><td>Device</td><td>${escapeHtml(g.device || '-')}</td></tr>
          <tr><td>Note</td><td style="font-size:11px;">${escapeHtml(g.note || '-')}</td></tr>
        </table>
      </div>
      <div class="card">
        <div class="card-title">Kubernetes / Containers</div>
        <table class="data-table">
          <tr><td>Status</td><td>${pill(k.enabled, 'Enabled', 'Disabled')}</td></tr>
          <tr><td>Namespace</td><td>${escapeHtml(k.namespace || '-')}</td></tr>
          <tr><td>Note</td><td style="font-size:11px;">${escapeHtml(k.note || '-')}</td></tr>
        </table>
      </div>
    </div>`;
}

// Company lookup via /api/company
async function lookupCompany() {
    const ticker = document.getElementById('ticker').value.trim().toUpperCase();
    const cikVal = document.getElementById('cik').value.trim();
    const useCik = document.getElementById('use-cik').checked;
    const id     = useCik ? cikVal : ticker;

    if (!id) { showAlert('Enter a ticker or CIK first.', 'warning'); return; }
    if (!isConnected && !isDemoMode) { showAlert(I18n.t('alert.not_connected_short'), 'error'); return; }
    if (isDemoMode) { showAlert('Company lookup not available in demo mode.', 'info'); return; }

    try {
        const param = useCik ? `cik=${encodeURIComponent(id)}` : `ticker=${encodeURIComponent(id)}`;
        const res   = await fetch(`${apiBaseUrl}/api/company?${param}`);
        if (res.ok) {
            const d = await res.json();
            showAlert(`${escapeHtml(d.name)} | Ticker: ${d.ticker} | CIK: ${d.cik} | SIC: ${d.sic}`, 'success');
        } else {
            showAlert('Company not found.', 'warning');
        }
    } catch (e) {
        showAlert('Lookup error: ' + e.message, 'error');
    }
}

// Export via server routes (JSON/CSV/HTML) with client-side fallback
async function exportViaServer(format) {
    if (!currentData) { showAlert('No data to export. Run an analysis first.', 'warning'); return; }
    if (!isConnected || isDemoMode) { exportResultsClientSide(format); return; }

    const company = currentData.company;
    const id      = company?.ticker || company?.cik;
    if (!id) { exportResultsClientSide(format); return; }

    const years   = document.getElementById('scope')?.value || '5';
    const useCik  = document.getElementById('use-cik')?.checked;
    const param   = useCik ? `cik=${encodeURIComponent(company.cik)}` : `ticker=${encodeURIComponent(company.ticker)}`;
    const url     = `${apiBaseUrl}/api/export/${format}?${param}&years=${years}`;

    try {
        const res = await fetch(url);
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        const blob     = await res.blob();
        const blobUrl  = URL.createObjectURL(blob);
        const fname    = `${id}_analysis.${format}`;
        const a        = document.createElement('a');
        a.href         = blobUrl;
        a.download     = fname;
        a.click();
        URL.revokeObjectURL(blobUrl);
        showAlert(`Exported ${fname} via server.`, 'success');
    } catch (e) {
        Logger.warning(`Server export failed (${e.message}), falling back to client-side export`);
        exportResultsClientSide(format);
    }
}


// Modal Management
function closeModal(modalId) {
    document.getElementById(modalId)?.classList.remove('active');
}

function showHelp() {
    document.getElementById('help-modal').classList.add('active');
}

function showAbout() {
    document.getElementById('about-modal').classList.add('active');
}

function showKeyboardShortcuts() {
    document.getElementById('shortcuts-modal').classList.add('active');
}

// ---------------------------------------------------------------------------
// Log Viewer
// ---------------------------------------------------------------------------
let _logsRawLines   = [];     // all lines from last fetch
let _logsAutoRefreshTimer = null;

async function showLogs() {
    document.getElementById('logs-modal').classList.add('active');
    await refreshLogs();
}

async function refreshLogs() {
    const pre    = document.getElementById('logs-content');
    const status = document.getElementById('logs-status');
    const lines  = document.getElementById('logs-lines')?.value || '200';

    if (!isConnected && !isDemoMode) {
        pre.textContent = '(Not connected to server — cannot retrieve logs.)';
        return;
    }

    if (isDemoMode) {
        _logsRawLines = [
            '2026-04-12 10:00:00.001 [INFO ] Starting Metis SEC EDGAR Fraud Analyzer v' + APP_VERSION,
            '2026-04-12 10:00:00.010 [INFO ] Log level: debug',
            '2026-04-12 10:00:00.011 [INFO ] Port: 8080',
            '2026-04-12 10:00:00.012 [INFO ] Server running on http://localhost:8080',
            '2026-04-12 10:00:01.100 [DEBUG] 127.0.0.1 GET /api/health -> 200',
            '2026-04-12 10:00:05.200 [INFO ] Looking up company by ticker: AAPL',
            '2026-04-12 10:00:05.800 [INFO ] Found company: Apple Inc. (CIK: 0000320193)',
            '2026-04-12 10:00:06.100 [DEBUG] 127.0.0.1 GET /api/analyze -> 200',
            '(Demo mode — showing sample log lines)'
        ];
        _renderLogs();
        if (status) status.textContent = 'Demo mode: ' + _logsRawLines.length + ' sample lines';
        return;
    }

    pre.textContent = 'Loading…';
    try {
        const res = await fetch(`${apiBaseUrl}/api/logs?lines=${lines}`);
        if (!res.ok) throw new Error('HTTP ' + res.status);
        const data = await res.json();

        if (data.error) {
            pre.textContent = 'Error: ' + data.error;
            return;
        }

        _logsRawLines = data.lines || [];
        _renderLogs();

        if (status) {
            status.textContent =
                `Showing ${data.returned} of ${data.total_lines} lines  |  ${data.path}`;
        }
    } catch (e) {
        pre.textContent = 'Failed to load logs: ' + e.message;
    }
}

function _renderLogs() {
    const pre      = document.getElementById('logs-content');
    const filter   = (document.getElementById('logs-filter')?.value || '').toLowerCase();
    const autoScroll = document.getElementById('logs-autoscroll')?.checked;

    const filtered = filter
        ? _logsRawLines.filter(l => l.toLowerCase().includes(filter))
        : _logsRawLines;

    // Colour-code by level
    const html = filtered.map(line => {
        const escaped = line
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;');

        if (/\[CRIT\s*\]|\[ERROR\]/.test(line))
            return `<span style="color:var(--danger);">${escaped}</span>`;
        if (/\[WARN\s*\]/.test(line))
            return `<span style="color:var(--warning);">${escaped}</span>`;
        if (/\[INFO\s*\]/.test(line))
            return `<span style="color:var(--dark);">${escaped}</span>`;
        if (/\[DEBUG\]/.test(line))
            return `<span style="color:var(--gray);">${escaped}</span>`;
        return escaped;
    }).join('\n');

    pre.innerHTML = html || '<span style="color:var(--gray);">(no matching lines)</span>';

    if (autoScroll) {
        pre.scrollTop = pre.scrollHeight;
    }
}

function filterLogs() {
    _renderLogs();
}

function toggleAutoRefresh() {
    const checked = document.getElementById('logs-autorefresh')?.checked;
    if (_logsAutoRefreshTimer) {
        clearInterval(_logsAutoRefreshTimer);
        _logsAutoRefreshTimer = null;
    }
    if (checked) {
        _logsAutoRefreshTimer = setInterval(() => {
            // Only refresh if the modal is open
            if (document.getElementById('logs-modal')?.classList.contains('active')) {
                refreshLogs();
            } else {
                clearInterval(_logsAutoRefreshTimer);
                _logsAutoRefreshTimer = null;
            }
        }, 5000);
    }
}

function downloadLogs() {
    if (_logsRawLines.length === 0) {
        showAlert(I18n.t('alert.no_logs'), 'warning');
        return;
    }
    const content  = _logsRawLines.join('\n');
    const filename = 'metis-server-' + new Date().toISOString().replace(/[:.]/g, '-') + '.log';
    downloadFile(content, filename, 'text/plain');
    showAlert(I18n.t('alert.logs_downloaded') + filename, 'success');
}

async function clearServerLogs() {
    if (!isConnected || isDemoMode) {
        showAlert(I18n.t('alert.not_connected_short'), 'error');
        return;
    }
    if (!confirm(I18n.t('confirm.clear_log'))) return;
    try {
        const res = await fetch(`${apiBaseUrl}/api/logs/clear`, { method: 'POST' });
        if (res.ok) {
            _logsRawLines = [];
            _renderLogs();
            showAlert(I18n.t('alert.logs_cleared'), 'success');
        } else {
            showAlert(I18n.t('alert.logs_clear_fail') + res.status, 'error');
        }
    } catch (e) {
        showAlert('Error: ' + e.message, 'error');
    }
}

// Alert Management
function showAlert(message, type = 'info') {
    const alertArea = document.getElementById('alert-area');
    const id = 'alert-' + Date.now();
    
    const alert = document.createElement('div');
    alert.id = id;
    alert.className = `alert alert-${type}`;
    alert.innerHTML = `
        <span>${escapeHtml(message)}</span>
        <button class="alert-close" onclick="dismissAlert('${id}')">&times;</button>
    `;
    
    alertArea.appendChild(alert);
    
    // Auto-dismiss after 5 seconds
    setTimeout(() => dismissAlert(id), 5000);
}

function dismissAlert(id) {
    const alert = document.getElementById(id);
    if (alert) {
        alert.remove();
    }
}

// Utility Functions
function escapeHtml(str) {
    if (!str) return '';
    const div = document.createElement('div');
    div.textContent = str;
    return div.innerHTML;
}

function formatCurrency(value) {
    if (value === undefined || value === null) return '-';
    const num = parseFloat(value);
    if (isNaN(num)) return '-';
    
    if (Math.abs(num) >= 1e9) {
        return '$' + (num / 1e9).toFixed(2) + 'B';
    } else if (Math.abs(num) >= 1e6) {
        return '$' + (num / 1e6).toFixed(2) + 'M';
    } else if (Math.abs(num) >= 1e3) {
        return '$' + (num / 1e3).toFixed(2) + 'K';
    }
    return '$' + num.toFixed(2);
}

function getRiskClass(level) {
    const l = (level || '').toUpperCase();
    if (l === 'LOW') return 'risk-low';
    if (l === 'MODERATE') return 'risk-moderate';
    if (l === 'ELEVATED') return 'risk-elevated';
    if (l === 'HIGH') return 'risk-high';
    if (l === 'CRITICAL') return 'risk-critical';
    return '';
}

function getBgClass(level) {
    const l = (level || '').toUpperCase();
    if (l === 'LOW') return 'bg-low';
    if (l === 'MODERATE') return 'bg-moderate';
    if (l === 'ELEVATED') return 'bg-elevated';
    if (l === 'HIGH') return 'bg-high';
    if (l === 'CRITICAL') return 'bg-critical';
    return '';
}

function getTrendArrow(trend) {
    if (trend === 'IMPROVING') return '^'; // Up arrow ASCII
    if (trend === 'DECLINING') return 'v'; // Down arrow ASCII
    return '-'; // Stable
}

function downloadFile(content, filename, type) {
    const blob = new Blob([content], { type: type });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    a.click();
    URL.revokeObjectURL(url);
}

// Demo Data Generator
function generateDemoData(identifier) {
    const isEnron = identifier.toUpperCase().includes('ENRON') || identifier === '0001024401';
    
    return {
        ticker: identifier,
        company: {
            name: isEnron ? 'Enron Corporation' : `${identifier} Inc.`,
            ticker: isEnron ? 'ENE' : identifier,
            cik: isEnron ? '0001024401' : '0000000000',
            sic: '4911'
        },
        filings_analyzed: 8,
        overall_risk: {
            score: isEnron ? 0.85 : 0.25,
            level: isEnron ? 'CRITICAL' : 'LOW'
        },
        models: {
            beneish: {
                m_score: isEnron ? -1.42 : -2.85,
                dsri: isEnron ? 1.35 : 1.02,
                gmi: isEnron ? 1.15 : 0.98,
                aqi: isEnron ? 1.28 : 1.01,
                sgi: isEnron ? 1.45 : 1.08
            },
            altman: {
                z_score: isEnron ? 1.25 : 3.45,
                zone: isEnron ? 'Distress' : 'Safe'
            },
            piotroski: {
                f_score: isEnron ? 2 : 7
            },
            fraud_triangle: {
                risk_score: isEnron ? 0.78 : 0.22,
                risk_level: isEnron ? 'HIGH' : 'LOW',
                pressure: isEnron ? 0.82 : 0.25,
                opportunity: isEnron ? 0.75 : 0.20,
                rationalization: isEnron ? 0.68 : 0.18
            },
            benford: {
                deviation: isEnron ? 8.5 : 2.1,
                suspicious: isEnron
            }
        },
        filings: [
            { accession: '0001024401-00-000123', filed_date: '2000-03-15', form_type: '10-K', revenue: isEnron ? 100789000000 : 50000000000, net_income: isEnron ? 979000000 : 5000000000, risk_level: isEnron ? 'HIGH' : 'LOW' },
            { accession: '0001024401-99-000456', filed_date: '1999-03-15', form_type: '10-K', revenue: isEnron ? 40112000000 : 45000000000, net_income: isEnron ? 893000000 : 4500000000, risk_level: isEnron ? 'MODERATE' : 'LOW' }
        ],
        trends: {
            revenue_trend: isEnron ? 'IMPROVING' : 'STABLE',
            income_trend: isEnron ? 'DECLINING' : 'IMPROVING',
            cash_flow_trend: isEnron ? 'DECLINING' : 'STABLE',
            debt_trend: isEnron ? 'DECLINING' : 'IMPROVING'
        },
        red_flags: isEnron ? [
            { type: 'EARNINGS_MANIPULATION', title: 'Beneish M-Score Above Threshold', description: 'M-Score of -1.42 exceeds the -2.22 threshold, indicating likely earnings manipulation.' },
            { type: 'BANKRUPTCY_RISK', title: 'Altman Z-Score in Distress Zone', description: 'Z-Score of 1.25 indicates high probability of bankruptcy within 2 years.' },
            { type: 'WEAK_FUNDAMENTALS', title: 'Low Piotroski F-Score', description: 'F-Score of 2 indicates weak financial fundamentals.' },
            { type: 'FRAUD_TRIANGLE', title: 'High Fraud Triangle Risk', description: 'Multiple fraud risk factors detected: high pressure, opportunity, and rationalization scores.' },
            { type: 'BENFORD_ANOMALY', title: 'Benford\'s Law Deviation', description: 'Significant deviation (8.5%) from expected digit distribution in financial figures.' }
        ] : [],
        recommendation: isEnron 
            ? 'CRITICAL RISK: Multiple fraud indicators detected. This analysis reflects Enron\'s actual financial condition prior to its 2001 collapse. The company filed for bankruptcy in December 2001.'
            : 'LOW RISK: No significant fraud indicators detected. Financial statements appear consistent with expected patterns.'
    };
}
