# Metis SEC EDGAR Fraud Analyzer - Changelog

**Version:** 3.1.3
**Author:** Bennie Shearer (Retired)
**Copyright (c) 2026 Bennie Shearer (Retired)**

**DISCLAIMER: This project is NOT funded, endorsed, or approved by the U.S. Securities and Exchange Commission (SEC).**

---

## [3.1.3] - 2026-04-12

### Fixed
- FIX-1: Removed circular include dependency — `RiskWeights` moved back into
  `types.h` as a plain data struct (stdlib-only includes). `config.h` now
  includes `types.h` to obtain it. Every TU that needed only `types.h` no
  longer transitively pulls in `AppConfig`, `Pson`, or `version.h`.
- FIX-2: Changed `setup_routes()` parameter from `const AppConfig&` to
  `AppConfig` (by value). All eight `[&cfg]` lambda captures in route handlers
  changed to `[cfg]` (value copy), eliminating the potential dangling-reference
  UB if lambdas outlive the caller's stack frame.
- FIX-3: Wired all 15 server REST routes into the browser client:
  - `exportViaServer(format)` calls `/api/export/{json,csv,html}` with
    automatic client-side fallback; `exportResults()` delegates to it.
  - `showServerInfo()` fetches `/api/version`, `/api/models`, `/api/weights`,
    `/api/cache/stats`, `/api/gpu/status`, `/api/k8s/status` in parallel
    and renders a Server Info dashboard modal.
  - `lookupCompany()` calls `/api/company`.
  - Server Info modal added to `index.html` (F2 / Help > Server Info).
  - Lookup Company and Cache Stats entries added to Tools menu.
  - F2 shortcut added to keyboard shortcuts table.
  - `exportResultsClientSide()` renamed from `exportResults()` to avoid
    infinite recursion in the `exportViaServer` fallback path.

---

## [3.0.0] - 2026-04-12

### Added
- Combined server and web client into single unified project
- PSON configuration format (config.pson) replacing all JSON/command-line parameters
- New `include/sec_analyzer/pson.h` - zero-dependency PSON parser
- New `include/sec_analyzer/config.h` - fully typed AppConfig structure loaded from config.pson
- New `src/config.cpp` - config translation unit
- New REST endpoints: GET /api/version, GET /api/models, GET /api/weights,
  GET /api/search, GET /api/cache/stats, GET /api/export/json,
  GET /api/gpu/status, GET /api/k8s/status
- GPU stub endpoint (reserved for future C++20 GPU backend)
- Kubernetes/container stub endpoint (reserved for future C++20 deployment)
- VERSION file drives CMake configure_file → version.h
- CMake POST_BUILD step copies client/ to bin/client/ automatically
- config.pson documents all parameters with inline comments
- All runtime parameters (port, paths, weights, SEC user agent, etc.) now in config.pson

### Changed
- Project renamed from Sec_Fraud_Analyzer_Server to Metis_SEC_EDGAR_Fraud_Analyzer
- CMake project: Metis_SEC_EDGAR_Fraud_Analyzer; binary: metis-sec-edgar-fraud-analyzer
- Static directory changed from "web" to "client" (matches web project)
- /api/cik/search unified with /api/search in both server and client
- Version bumped to 3.0.0 across all files
- All version strings now driven by VERSION file via CMake configure_file
- RiskWeights and ServerConfig moved from types.h to config.h
- server_header string uses SEC_ANALYZER_VERSION_STRING macro
- Client app.js version updated from 2.2.0 to 3.1.3
- docs/ directory contains all .md and .txt documentation

### Recommended Improvements (Implemented)
- RI-01: PSON config format replaces JSON/CLI config mixing
- RI-02: VERSION file as single source of truth for version
- RI-03: Unified project combining server + client
- RI-04: GPU stub endpoint for future C++20 CUDA/OpenCL/SYCL implementation
- RI-05: Kubernetes/container stub for future C++20 deployment
- RI-06: /api/version endpoint exposing app metadata
- RI-07: /api/models endpoint listing available fraud models
- RI-08: /api/weights endpoint exposing current model weights
- RI-09: /api/cache/stats endpoint
- RI-10: /api/export/json endpoint (alongside existing csv/html)

---

## [2.2.0] - 2026-01-23  (Web Client - last standalone version)

### Added
- EXAMPLES.md - Usage examples and sample outputs
- FAQ.md - Frequently asked questions
- GLOSSARY.md - Financial and technical terms glossary
- Exit button in File menu (Alt+F4)

---

## [2.1.2] - 2026-01-23  (Server - last standalone version)

### Added
- Limitations section to all documentation
- Acknowledgments section to all documentation
- XBRL coverage and data availability notes

---
