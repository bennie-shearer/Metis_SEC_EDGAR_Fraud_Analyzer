# Metis SEC EDGAR Fraud Analyzer

**Version:** 3.1.3 &nbsp;|&nbsp; **License:** MIT &nbsp;|&nbsp; **Platform:** Windows · Linux · macOS &nbsp;|&nbsp; **Language:** C++20

> **DISCLAIMER:** This project is NOT funded, endorsed, or approved by the
> U.S. Securities and Exchange Commission (SEC).
> For educational and research purposes only.

---

A cross-platform C++20 financial forensics server with a built-in single-page browser client. It fetches public SEC EDGAR filings and applies five academically-validated fraud detection models to produce a composite risk score and actionable red flags — all from a single self-contained executable with zero external dependencies.

---

## Features

| | |
|---|---|
| **Five fraud models** | Beneish M-Score · Altman Z-Score · Piotroski F-Score · Fraud Triangle · Benford's Law |
| **Composite scoring** | Configurable per-model weights, auto-normalized |
| **Browser client** | Single-page app — dark mode, 9 locales, log viewer, demo mode, batch analysis |
| **REST API** | 19 endpoints covering analysis, export, cache, logs, and server control |
| **CLI mode** | Analyze a company and write JSON / CSV / HTML without starting the server |
| **PSON config** | Every runtime parameter in one plain-text `config.pson` file |
| **Zero dependencies** | WinHTTP on Windows; system `curl` on Linux/macOS; no Boost, no OpenSSL |
| **i18n** | 9 locales: `en` `en-GB` `fr` `de` `es` `ja` `zh` `ko` `pt` |

---

## Quick Start

### 1 — Build

**Windows (MinGW-w64 GCC 13, CLion)**

```
cmake -S . -B cmake-build-release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release
```

**Linux / macOS**

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j$(nproc)
```

The executable is written to `cmake-build-release/bin/`. CMake automatically
copies `client/`, `config.pson`, and `docs/` next to it on every build.

### 2 — Configure

Edit `cmake-build-release/bin/config.pson` (also edit the project-root
`config.pson` so the next build does not overwrite your change):

```
# SEC EDGAR User-Agent — required format: FirstName LastName email@domain.com
# Do NOT use parentheses or an app/version prefix — causes HTTP 403
sec.user_agent=Bob Doe bob@doe.com

# Adjust if you run on a different port
server.port=8080
client.api_url=http://localhost:8080
```

### 3 — Run

```bash
cd cmake-build-release/bin
./metis-sec-edgar-fraud-analyzer          # Linux/macOS
metis-sec-edgar-fraud-analyzer.exe        # Windows
```

Open **http://localhost:8080** in your browser. The status indicator in the
top-right corner should show **Connected**.

### 4 — CLI mode

```bash
# Analyze Apple, print JSON
./metis-sec-edgar-fraud-analyzer --ticker AAPL --years 5 --format json

# Analyze a delisted company by CIK, save an HTML report
./metis-sec-edgar-fraud-analyzer --cik 0001024401 --format html > enron.html
```

---

## SEC User-Agent

The SEC EDGAR API requires a `User-Agent` in this **exact** format or returns HTTP 403:

```
FirstName LastName email@domain.com
```

Examples:

```
sec.user_agent=Bob Doe bob@doe.com          ✓ correct
sec.user_agent=Jane Smith jane@example.org  ✓ correct
sec.user_agent=MyApp/1.0 (bob@doe.com)      ✗ wrong — parentheses not accepted
sec.user_agent=contact@example.com          ✗ wrong — no name
```

---

## REST API

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/api/health` | Server health + cache entry count |
| `GET` | `/api/version` | Application version and build metadata |
| `GET` | `/api/models` | Available fraud detection models |
| `GET` | `/api/weights` | Current model weights from config |
| `GET` | `/api/company?ticker=` | Company lookup by ticker or CIK |
| `GET` | `/api/analyze?ticker=&years=` | Full fraud analysis (cached) |
| `GET` | `/api/filings?ticker=&years=` | SEC filing list |
| `GET` | `/api/search?q=` | Company name/ticker search |
| `GET` | `/api/export/json\|csv\|html` | Server-rendered file download |
| `GET` | `/api/cache/stats` | Cache entries and TTL |
| `POST` | `/api/cache/clear` | Clear in-memory cache |
| `GET` | `/api/logs?lines=N` | Tail of server log file |
| `POST` | `/api/logs/clear` | Truncate server log file |
| `GET` | `/api/gpu/status` | GPU backend status (reserved) |
| `GET` | `/api/k8s/status` | Kubernetes status (reserved) |
| `POST` | `/api/shutdown` | Graceful server stop |
| `GET` | `/config.pson` | Serves live config to browser client |

Full documentation: [`docs/API.md`](docs/API.md)

---

## Fraud Detection Models

| Model | Author | Purpose | Flag threshold |
|-------|--------|---------|----------------|
| **Beneish M-Score** | Messod Beneish (1999) | Earnings manipulation | M > −2.22 |
| **Altman Z-Score** | Edward Altman (1968) | Bankruptcy risk | Z < 1.81 distress zone |
| **Piotroski F-Score** | Joseph Piotroski (2000) | Financial strength (0–9) | F ≤ 2 weak |
| **Fraud Triangle** | Donald Cressey (1953) | Pressure / opportunity / rationalization | composite |
| **Benford's Law** | Frank Benford (1938) | Digit distribution anomaly | MAD > 0.015 |

Model weights are fully configurable in `config.pson` (`weights.*` keys) and
are auto-normalized if they do not sum to 1.0.

Full methodology: [`docs/MODELS.md`](docs/MODELS.md)

---

## Configuration Reference

All runtime parameters live in `config.pson`. Key settings:

| Key | Default | Description |
|-----|---------|-------------|
| `server.port` | `8080` | HTTP listen port |
| `server.host` | `0.0.0.0` | Listen address |
| `server.threads` | `4` | Worker thread count |
| `paths.static_dir` | `client` | Browser client directory |
| `paths.log_file` | `logs/metis.log` | Log output path |
| `logging.level` | `debug` | `debug` · `info` · `warning` · `error` · `critical` |
| `logging.console` | `true` | Echo log lines to stdout |
| `sec.user_agent` | `Bob Doe bob@doe.com` | SEC EDGAR User-Agent (see above) |
| `sec.rate_limit_ms` | `100` | Delay between SEC requests |
| `sec.timeout_seconds` | `30` | HTTP request timeout |
| `cache.ttl_seconds` | `3600` | Analysis cache TTL |
| `analysis.default_years` | `5` | Default analysis window |
| `weights.beneish` | `0.30` | Beneish model weight |
| `weights.altman` | `0.25` | Altman model weight |
| `weights.piotroski` | `0.15` | Piotroski model weight |
| `weights.fraud_triangle` | `0.15` | Fraud Triangle weight |
| `weights.benford` | `0.05` | Benford model weight |
| `weights.red_flags` | `0.10` | Red flags weight |
| `client.api_url` | `http://localhost:8080` | Browser auto-connect URL |
| `client.detect_browser_locale` | `false` | Auto-detect browser language |
| `gpu.enabled` | `false` | GPU backend (reserved) |
| `k8s.enabled` | `false` | Kubernetes deployment (reserved) |

---

## Project Structure

```
metis-sec-edgar-fraud-analyzer/
├── CMakeLists.txt              CMake build — reads VERSION, copies assets on build
├── VERSION                     Single version source of truth: 3.1.3
├── LICENSE                     MIT License
├── config.pson                 All runtime parameters
├── src/
│   ├── main.cpp                Entry point, REST routes, CLI mode
│   ├── http_server.cpp         Cross-platform HTTP/1.1 server
│   ├── sec_fetcher.cpp         SEC EDGAR client (WinHTTP / curl)
│   ├── analyzer.cpp            Analysis orchestration
│   ├── exporter.cpp            JSON / CSV / HTML export
│   ├── cache.cpp               In-memory TTL cache
│   └── models/                 One .cpp per fraud model
├── include/sec_analyzer/
│   ├── version.h.in            CMake template → version.h
│   ├── pson.h                  PSON parser (header-only, zero deps)
│   ├── config.h                Typed AppConfig loaded from PSON
│   ├── types.h                 Core data structures
│   ├── logger.h                Thread-safe file + console logger
│   ├── json.h                  JSON parser/builder (header-only, zero deps)
│   └── models/                 One .h per fraud model
├── client/
│   ├── index.html              Single-page browser client
│   ├── app.js                  Client logic, i18n wiring
│   ├── i18n.js                 9-locale translation system
│   └── style.css               CSS with dark mode
└── docs/                       Full documentation suite
    ├── API.md                  REST API reference
    ├── MODELS.md               Fraud model methodology
    ├── BUILD.md                Detailed build instructions
    ├── HOWTO.md                Step-by-step task guide
    ├── USER_GUIDE.md           End-user guide
    ├── CHANGELOG.md            Full version history
    ├── CONTRIBUTING.md         Contribution conventions
    ├── TROUBLESHOOTING.md      Common issues and fixes
    ├── FAQ.md                  Frequently asked questions
    ├── SECURITY.md             Security scope and hardening
    ├── BACKGROUND.md           Architecture and design rationale
    ├── EXAMPLES.md             Usage examples
    └── GLOSSARY.md             Financial and technical terminology
```

---

## Internationalization

The browser client supports 9 locales selectable from the menu bar:

| Code | Language |
|------|----------|
| `en` | English (American) — default |
| `en-GB` | English (British) |
| `fr` | French |
| `de` | German |
| `es` | Spanish |
| `ja` | Japanese |
| `zh` | Chinese (Simplified) |
| `ko` | Korean |
| `pt` | Portuguese (Brazilian) |

American English is always the default. Browser language auto-detection is
opt-in via `client.detect_browser_locale=true` in `config.pson`.

---

## Building on Windows

Tested with **MinGW-w64 GCC 13** on Windows 10/11. The recommended workflow is CLion.

- No Visual Studio required
- No WSL required
- WinHTTP is used for all HTTPS requests — no OpenSSL dependency
- Static MinGW runtime is recommended to avoid DLL deployment issues

See [`docs/BUILD.md`](docs/BUILD.md) for detailed Windows build instructions.

---

## Acknowledgments

- **Walter Hamscher** — Mentor and XBRL expert
- **SEC EDGAR** — Financial data source (https://www.sec.gov/developer)
- **Messod Beneish** — M-Score earnings manipulation detection
- **Edward Altman** — Z-Score bankruptcy prediction
- **Joseph Piotroski** — F-Score financial strength evaluation
- **Donald Cressey** — Fraud Triangle framework
- **Frank Benford** — Law of Anomalous Numbers
- **CLion** by JetBrains s.r.o.
- **Claude** by Anthropic PBC

---

## License

MIT License — Copyright (c) 2026 Bennie Shearer (Retired)

See [`LICENSE`](LICENSE) for the full license text.
