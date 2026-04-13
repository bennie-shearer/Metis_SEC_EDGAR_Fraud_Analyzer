# Metis SEC EDGAR Fraud Analyzer - TODO

**Version:** 3.1.3
**Author:** Bennie Shearer (Retired)
**Copyright (c) 2026 Bennie Shearer (Retired)**

**DISCLAIMER: This project is NOT funded, endorsed, or approved by the U.S. Securities and Exchange Commission (SEC).**

---

## Open Items

### High Priority

- [ ] **TLS / HTTPS support** — add optional TLS termination using platform
  APIs (SChannel on Windows, Secure Transport on macOS, OpenSSL optional on
  Linux) so the server can be deployed without a reverse proxy.

- [ ] **XBRL data quality improvements** — the current XBRL extractor uses
  fixed concept names (`Revenues`, `Assets`, etc.) and falls back to zero when
  a company uses non-standard tags. Implement a ranked synonym list per concept
  to improve coverage for non-manufacturing and foreign filers (20-F).

- [ ] **Industry-adjusted model thresholds** — Beneish M-Score and Altman
  Z-Score thresholds were calibrated on manufacturing companies. Add SIC-code
  lookup tables to apply appropriate zone boundaries for financial institutions,
  utilities, technology, and foreign private issuers.

- [ ] **Persistent file cache** — the current `FileCache` implementation in
  `src/cache.cpp` has a stub `clear()` and no TTL expiry scan. Implement proper
  directory iteration, per-entry timestamp metadata, and size-capped eviction so
  analyses survive server restarts.

### Medium Priority

- [ ] **WebSocket streaming** — replace the polling pattern in the log viewer
  and (future) real-time analysis progress with a proper RFC 6455 WebSocket
  upgrade. Add `HttpServer::upgrade_websocket()` in C++20.

- [ ] **Second-digit Benford analysis** — `BenfordSecondDigitModel` is fully
  implemented in `benford.cpp` but never called by the analyzer or exposed in
  the API response. Wire it into `FraudAnalyzer::analyze_financials()` and add
  a `benford_second` field to the JSON output and client display.

- [ ] **Batch analysis server-side** — client-side batch analysis serializes
  calls from the browser. Add `POST /api/batch` accepting a JSON array of
  tickers/CIKs and returning a streamed NDJSON response, enabling true parallel
  server-side analysis.

- [ ] **NDJSON / structured logging** — replace the current human-readable log
  format with optional NDJSON output (`logging.format=ndjson` in config.pson)
  for integration with log aggregators (Elasticsearch, Splunk, Loki).

- [ ] **Pagination for /api/filings** — the current endpoint returns up to 100
  filings. Add `offset` and `limit` query parameters and include `total_count`
  in the response for proper UI pagination.

- [ ] **Market cap lookup** — Altman X4 currently falls back to book equity when
  market cap is unavailable. Integrate a lightweight free market data source
  (e.g. Yahoo Finance JSON API) to supply real-time or last-close market cap
  for the X4 ratio.

### Low Priority / Future

- [ ] **GPU backend (C++20 SYCL stub)** — the `gpu.enabled` config key and
  `GET /api/gpu/status` endpoint are reserved. Implement a SYCL/DPC++ kernel
  for parallel Benford distribution computation and large-matrix financial
  ratio batch processing.

- [ ] **Kubernetes / container deployment** — the `k8s.enabled` key and
  `GET /api/k8s/status` endpoint are reserved. Add Prometheus `/metrics`
  endpoint, Kubernetes readiness/liveness probe routes, and horizontal pod
  autoscaling configuration (no Docker required — static binary packaging).

- [ ] **Authentication** — add optional Bearer token or API-key authentication
  to all `/api/*` routes via `server.auth_token` in config.pson, so the server
  can be safely exposed on a local network without a reverse proxy.

- [ ] **CHANGELOG viewer in client** — fetch and render `docs/CHANGELOG.md`
  inside the About modal (similar to the existing log viewer pattern).

- [ ] **Dark mode persistence via config.pson** — currently `client.dark_mode`
  is written to `localStorage` but not back to `config.pson`. Add a
  **Save Settings** menu item that calls `saveConfigFile()` so preferences
  persist across machines and browser clears.

- [ ] **Configurable Benford threshold** — `MAD_MARGINALLY_ACCEPTABLE` is
  hard-coded at `0.015`. Expose it as `analysis.benford_mad_threshold` in
  config.pson so users can tune sensitivity.

- [ ] **SEC rate-limit backoff** — when the SEC returns HTTP 429, the fetcher
  logs an error and gives up. Add exponential backoff with a configurable max
  retry count (`sec.max_retries`, `sec.backoff_ms` in config.pson).

- [ ] **Linux/macOS native HTTPS** — the Linux/macOS SEC fetcher spawns `curl`
  via `popen()`. Replace this with a native `getaddrinfo` + OpenSSL/Secure
  Transport socket to remove the `curl` dependency.

---

## Completed

- [x] Combined server + web client into single unified project (v3.0.0)
- [x] PSON configuration format — all parameters in config.pson (v3.0.0)
- [x] VERSION file as single source of truth (v3.0.0)
- [x] GPU and Kubernetes stub endpoints (v3.0.0)
- [x] All 15 REST routes wired into browser client (v3.0.1)
- [x] Server Info dashboard modal (F2) (v3.0.1)
- [x] Server-side export endpoints for JSON/CSV/HTML (v3.0.1)
- [x] Fixed types.h / config.h circular include (v3.0.1)
- [x] cfg lambda capture changed from &ref to value (v3.0.1)
- [x] macOS __add_alignment_assumption fix in benford.h (v3.0.3)
- [x] Complete logging — all config values logged at startup (v3.0.3)
- [x] Dark/Light mode toolbar button with dynamic icon (v3.0.3)
- [x] Exit/Shutdown toolbar button + POST /api/shutdown (v3.0.3)
- [x] Log Viewer modal (F3) with filter, colour-coding, auto-refresh (v3.1.3)
- [x] GET /api/logs and POST /api/logs/clear endpoints (v3.1.3)
- [x] Client PSON parser — config.pson read correctly by browser (v3.1.3)
- [x] client.api_url in config.pson eliminates manual connection setup (v3.1.3)
