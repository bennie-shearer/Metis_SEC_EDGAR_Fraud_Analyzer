# Metis SEC EDGAR Fraud Analyzer - How-To Guide

**Version:** 3.1.3
**Author:** Bennie Shearer (Retired)
**Copyright (c) 2026 Bennie Shearer (Retired)**

**DISCLAIMER: This project is NOT funded, endorsed, or approved by the U.S. Securities and Exchange Commission (SEC).**

---


### SEC User-Agent format

The SEC EDGAR API requires this exact format — no parentheses, no app name, no version:

```
FirstName LastName email@domain.com
```

Examples of **correct** values:
```
sec.user_agent=Bob Doe bob@doe.com
sec.user_agent=Jane Smith jane.smith@example.org
```

Examples of **incorrect** values that cause HTTP 403:
```
sec.user_agent=MyApp/1.0 (bob@doe.com)
sec.user_agent=contact@example.com
```

## Quick Start

### 1. Build

**Windows (MinGW-w64 GCC 13, CLion)**
```
mkdir cmake-build-release
cd cmake-build-release
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j4
```

**Linux / macOS**
```bash
mkdir cmake-build-release && cd cmake-build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Output: `cmake-build-release/bin/metis-sec-edgar-fraud-analyzer[.exe]`  
Client files are automatically copied to: `cmake-build-release/bin/client/`

### 2. Configure

Copy `config.pson` next to the executable and edit it:

```
# Mandatory — SEC requires: FirstName LastName email@domain.com
# Do NOT use parentheses or an app/version prefix — causes HTTP 403
sec.user_agent=Bob Doe bob@doe.com

# If running on a non-default port, update both the server and client values
server.port=8080
client.api_url=http://localhost:8080
```

The `client.api_url` line is how the browser knows where to connect.
If the server is on a remote host, change it to `http://hostname:8080`.

### 3. Run

```bash
cd cmake-build-release/bin
./metis-sec-edgar-fraud-analyzer
# or: ./metis-sec-edgar-fraud-analyzer --config /path/to/config.pson
```

Open `http://localhost:8080` in your browser. The status indicator in the top
right should change from **Not Connected** to **Connected**.

### 4. Analyze a Company

1. Enter a ticker symbol (e.g. `AAPL`, `MSFT`, `TSLA`) in the Stock Ticker field.
2. Select the analysis scope (3, 5, or 10 years).
3. Click **Analyze**.
4. Results appear across five tabs: Overview, Fraud Models, Filing Analysis,
   Trends, and Red Flags.

---

## How to Analyze a Delisted Company

Companies that are no longer traded (e.g. Enron, WorldCom) have no active
ticker. Use their SEC CIK number instead.

1. Check **Use CIK (for delisted)** to switch input mode.
2. Open **Tools > CIK Lookup** to search for the CIK number, or use a
   known value:
   - Enron Corp: `0001024401`
   - WorldCom: `0000723527`
   - Lehman Brothers: `0000806085`
3. Enter the CIK and click **Analyze**.

---

## How to Connect the Browser to the Server

The browser reads `config.pson` from the server on startup and uses
`client.api_url` to find the API. If you see **Not Connected**:

1. Verify the server is running — check the console or `logs/metis.log`.
2. Open **Connect > Connection Settings**.
3. Set **API Server URL** to `http://localhost:8080` (or the correct address).
4. Click **Save & Connect**.

The URL is saved in browser `localStorage` for future sessions.
Alternatively, set `client.api_url` in `config.pson` so it is applied
automatically on every page load.

---

## How to Export Results

**From the File menu or keyboard:**

| Action | Shortcut |
|--------|----------|
| Export JSON | Ctrl+S |
| Export CSV  | Ctrl+E |
| Export HTML report | Ctrl+H |
| Print | Ctrl+P |

The client first tries the server-side export routes (`/api/export/{json,csv,html}`)
for a server-rendered file, then falls back to a client-side download if the
server is unreachable.

---

## How to View Server Logs

Press **F3**, click the log icon in the toolbar, or open **Help > View Logs**.

- Use the **Lines** selector to control how many lines are fetched.
- Type in the **Filter** box to show only matching lines (e.g. `ERROR`, `AAPL`).
- Enable **Auto-refresh** to poll the server every 5 seconds.
- Click **Download** to save the current log view as a `.log` file.
- Click **Clear Log File** to truncate `logs/metis.log`.

Log files are written to the path set by `paths.log_file` in `config.pson`
(default: `logs/metis.log` relative to the executable).

---

## How to Use Demo Mode

Demo mode lets you explore the full client UI without a running server.

Toggle it with **Ctrl+M** or **View > Toggle Demo Mode**.

- Entering ticker `ENRON` (or CIK `0001024401`) shows a high-risk sample.
- Any other ticker shows a low-risk sample.
- Exports, batch analysis, and the log viewer all work in demo mode.

---

## How to Run in CLI Mode

Analyze a company without starting the HTTP server:

```bash
# By ticker, output JSON
./metis-sec-edgar-fraud-analyzer --ticker AAPL --years 5 --format json

# By CIK, output HTML report
./metis-sec-edgar-fraud-analyzer --cik 0001024401 --format html > enron.html

# With explicit config file
./metis-sec-edgar-fraud-analyzer --config /etc/metis/config.pson --ticker TSLA
```

**Format options:** `json` (default), `csv`, `html`

---

## How to Shut Down the Server

- **From the browser:** Click the red power button in the toolbar (or
  **File > Exit**). This calls `POST /api/shutdown` to stop the server, then
  closes the tab.
- **From the terminal:** Press `Ctrl+C`.
- **Programmatically:** `POST http://localhost:8080/api/shutdown`

---

## How to Change Model Weights

Edit `config.pson`. Weights are auto-normalized if they do not sum to 1.0:

```
weights.beneish=0.35
weights.altman=0.25
weights.piotroski=0.15
weights.fraud_triangle=0.15
weights.benford=0.05
weights.red_flags=0.05
```

Restart the server to apply changes. Current weights are visible at
`GET /api/weights` or via **Help > Server Info**.

---

## How to Change the Port

Edit `config.pson`:

```
server.port=9090
client.api_url=http://localhost:9090
```

Both values must be updated together. Restart the server.

---

## How to Run on a Remote Server

On the server machine, edit `config.pson`:
```
server.host=0.0.0.0
server.port=8080
```

On the client machine (or in `config.pson` that the browser loads):
```
client.api_url=http://192.168.1.100:8080
```

**Note:** There is no TLS/HTTPS support in this version. Use a reverse proxy
(nginx, Caddy) to add HTTPS for production deployments.

---

## Logging Reference

| Level | Meaning |
|-------|---------|
| `debug` | Full request/response trace + all diagnostics |
| `info` | Startup config, company lookups, analysis events |
| `warning` | Non-fatal issues (missing XBRL data, parse warnings) |
| `error` | Request failures, file errors |
| `critical` | Fatal errors (port bind failure, etc.) |

Set in `config.pson`: `logging.level=debug`  
Set console echo: `logging.console=true`
