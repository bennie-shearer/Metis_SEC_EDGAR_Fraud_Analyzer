# Metis SEC EDGAR Fraud Analyzer - User Guide

**Version:** 3.1.3
**Author:** Bennie Shearer (Retired)

**DISCLAIMER: This project is NOT funded, endorsed, or approved by the U.S. Securities and Exchange Commission (SEC).**

---

## Starting the Server

1. Edit `config.pson` and set your email in `sec.user_agent` (required by SEC).
2. Run the server:
   ```
   metis-sec-edgar-fraud-analyzer
   ```
3. Open `http://localhost:8080` in your browser.

---

## Analyzing a Company

**By Ticker (active companies)**
1. Enter a ticker symbol in the "Stock Ticker" field (e.g. `AAPL`, `MSFT`).
2. Select the analysis scope: 3, 5, or 10 years.
3. Click **Analyze**.

**By CIK (delisted / historical companies)**
1. Check "Use CIK (for delisted)" to switch input mode.
2. Enter the CIK number (e.g. `0001024401` for Enron).
3. Use **Tools > CIK Lookup** to search for CIK numbers.
4. Click **Analyze**.

---

## Reading the Results

### Overview Tab
- **Overall Risk Score** — composite percentage from 0% (no risk) to 100% (critical)
- **Risk Level** — LOW / MODERATE / ELEVATED / HIGH / CRITICAL
- **Filings Analyzed** — number of 10-K/10-Q filings processed
- **Red Flags Detected** — count of triggered warning conditions
- **Recommendation** — plain-language guidance

### Fraud Models Tab
Shows individual scores for all five models:
- Beneish M-Score with threshold indicator
- Altman Z-Score with zone (Safe / Gray / Distress)
- Piotroski F-Score (0–9 scale)
- Fraud Triangle composite risk percentage
- Benford's Law deviation percentage

### Filing Analysis Tab
Per-filing table with revenue, net income, form type, and filing date.

### Trends Tab
Direction indicators (improving / stable / declining) for:
- Revenue, Net Income, Cash Flow, Debt Ratio

### Red Flags Tab
Detailed list of triggered warning conditions with source model and description.

---

## Exporting Results

**File menu or keyboard shortcuts:**
- `Ctrl+S` — Export JSON (raw data)
- `Ctrl+E` — Export CSV (summary spreadsheet)
- `Ctrl+H` — Export HTML (standalone report)
- `Ctrl+P` — Print current view

The server also exposes direct export endpoints:
```
GET /api/export/json?ticker=AAPL
GET /api/export/csv?ticker=AAPL
GET /api/export/html?ticker=AAPL
```

---

## Demo Mode

Demo mode lets you explore the interface without a live server connection.
Toggle it with `Ctrl+M` or **View > Toggle Demo Mode**.

In demo mode, entering `ENRON` or CIK `0001024401` generates realistic
high-risk sample data matching Enron's pre-bankruptcy profile.

---

## Batch Analysis

**Tools > Batch Analysis** accepts multiple ticker symbols (one per line or
comma-separated) and runs sequential analyses with rate limiting.

---

## Connection Settings

**Connect > Connection Settings** (or click the status indicator):
- **API Server URL** — base URL only (e.g. `http://localhost:8080`). The
  client appends `/api/*` paths automatically.
- **Request Timeout** — seconds before a request is aborted.
- **Test Connection** — pings `/api/health` to verify connectivity.

Settings are persisted in browser `localStorage`.

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+S   | Export JSON |
| Ctrl+E   | Export CSV |
| Ctrl+H   | Export HTML |
| Ctrl+P   | Print report |
| Ctrl+D   | Toggle dark mode |
| Ctrl+M   | Toggle demo mode |
| F1       | Help |
| Escape   | Close modal |
| Alt+F4   | Exit |

---

## CLI Mode

Run a one-shot analysis without starting the HTTP server:

```bash
# By ticker
metis-sec-edgar-fraud-analyzer --ticker AAPL --years 5 --format json

# By CIK, output HTML
metis-sec-edgar-fraud-analyzer --cik 0001024401 --format html > enron.html

# Custom config
metis-sec-edgar-fraud-analyzer --config /etc/metis/config.pson --ticker TSLA
```

**Format options:** `json` (default), `csv`, `html`
