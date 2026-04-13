# Metis SEC EDGAR Fraud Analyzer - REST API Reference

**Version:** 3.1.3
**Author:** Bennie Shearer (Retired)

**DISCLAIMER: This project is NOT funded, endorsed, or approved by the U.S. Securities and Exchange Commission (SEC).**

---

## Base URL

```
http://localhost:8080
```

Port is configurable in `config.pson` (`server.port`).

---

## Endpoints

### GET /api/health

Server health check.

**Response**
```json
{
  "status": "healthy",
  "version": "3.1.3",
  "timestamp": "2026-04-12T15:00:00Z",
  "cache_entries": 4
}
```

---

### GET /api/version

Application version and metadata.

**Response**
```json
{
  "version": "3.1.3",
  "app_name": "Metis SEC EDGAR Fraud Analyzer",
  "copyright": "Copyright (c) 2026 Bennie Shearer (Retired)",
  "license": "MIT License",
  "disclaimer": "NOT funded, endorsed, or approved by the U.S. SEC"
}
```

---

### GET /api/models

Lists available fraud detection models with descriptions.

**Response**
```json
{
  "models": [
    { "id": "beneish", "name": "Beneish M-Score", "description": "..." },
    { "id": "altman",  "name": "Altman Z-Score",  "description": "..." },
    { "id": "piotroski","name": "Piotroski F-Score","description": "..." },
    { "id": "fraud_triangle","name": "Fraud Triangle","description": "..." },
    { "id": "benford", "name": "Benford's Law",   "description": "..." }
  ]
}
```

---

### GET /api/weights

Current model weights used in composite risk score calculation.

**Response**
```json
{
  "beneish": 0.30,
  "altman": 0.25,
  "piotroski": 0.15,
  "fraud_triangle": 0.15,
  "benford": 0.05,
  "red_flags": 0.10
}
```

---

### GET /api/company

Look up a company by ticker or CIK.

**Query Parameters**

| Parameter | Required | Description |
|-----------|----------|-------------|
| `ticker` | One of | Stock ticker symbol (e.g. `AAPL`) |
| `cik` | One of | SEC CIK number (e.g. `0001024401`) |

**Response**
```json
{
  "name": "Apple Inc.",
  "ticker": "AAPL",
  "cik": "0000320193",
  "sic": "3571"
}
```

---

### GET /api/analyze

Run a full fraud analysis for a company.

**Query Parameters**

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `ticker` | One of | — | Stock ticker symbol |
| `cik` | One of | — | SEC CIK number |
| `years` | No | `5` | Number of years to analyze |

**Response** — full `AnalysisResult` JSON object including:
- `company` — name, ticker, CIK, SIC
- `filings_analyzed` — count
- `overall_risk` — `score` (0.0–1.0), `level` (LOW/MODERATE/ELEVATED/HIGH/CRITICAL)
- `models` — beneish, altman, piotroski, fraud_triangle, benford sub-objects
- `red_flags` — array of detected issues
- `trends` — revenue, income, cash flow, debt direction
- `filings` — per-filing financial data
- `recommendation` — human-readable summary
- `version`, `analysis_timestamp`

---

### GET /api/filings

List SEC filings for a company without running analysis.

**Query Parameters**

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `ticker` | One of | — | Stock ticker symbol |
| `cik` | One of | — | SEC CIK number |
| `years` | No | `5` | Number of years |

**Response**
```json
{
  "filings": [
    {
      "accession": "0000320193-23-000006",
      "form_type": "10-K",
      "filed_date": "2023-11-03",
      "fiscal_year": 2023
    }
  ],
  "count": 10
}
```

---

### GET /api/search

Search for companies by name or ticker substring.

**Query Parameters**

| Parameter | Required | Description |
|-----------|----------|-------------|
| `q` | Yes | Search string |

**Response**
```json
{
  "results": [
    { "name": "Enron Corp", "ticker": "ENE", "cik": "0001024401" }
  ]
}
```

---

### GET /api/export/json

Download analysis result as a JSON file attachment.

**Query Parameters** — same as `/api/analyze`

**Response** — `Content-Type: application/json` with `Content-Disposition: attachment`

---

### GET /api/export/csv

Download analysis result as a CSV file attachment.

**Query Parameters** — same as `/api/analyze`

**Response** — `Content-Type: text/csv` with `Content-Disposition: attachment`

---

### GET /api/export/html

Download a standalone HTML report.

**Query Parameters** — same as `/api/analyze`

**Response** — `Content-Type: text/html`

---

### GET /api/cache/stats

Cache statistics.

**Response**
```json
{ "entries": 4, "ttl_seconds": 3600 }
```

---

### POST /api/cache/clear

Clear the in-memory analysis cache.

**Response**
```json
{ "status": "cleared" }
```

---

### GET /api/gpu/status

GPU backend status (reserved for future C++20 implementation).

**Response**
```json
{
  "enabled": false,
  "device": "auto",
  "status": "disabled",
  "note": "GPU acceleration is reserved for a future C++20 implementation"
}
```

---

### GET /api/k8s/status

Kubernetes/container deployment status (reserved).

**Response**
```json
{
  "enabled": false,
  "namespace": "default",
  "status": "disabled",
  "note": "Kubernetes/container deployment is reserved for a future C++20 implementation"
}
```

---

## Error Responses

All errors return JSON:

```json
{ "error": "description of the error" }
```

| HTTP Code | Meaning |
|-----------|---------|
| 400 | Bad request — missing required parameter |
| 404 | Company or resource not found |
| 500 | Internal server error — check server logs |

---

## CORS

CORS is enabled by default (`server.cors=true` in config.pson). All origins are
accepted. Set `server.cors=false` to disable.

---

### GET /api/logs

Returns the tail of the server log file.

**Query Parameters**

| Parameter | Default | Description |
|-----------|---------|-------------|
| `lines` | `200` | Number of lines from the end (max 5000) |
| `raw` | `0` | Set to `1` to return plain text instead of JSON |

**Response (JSON)**
```json
{
  "lines": ["2026-04-12 10:00:00.001 [INFO ] ..."],
  "total_lines": 450,
  "returned": 200,
  "path": "logs/metis.log"
}
```

**Response (raw text, `?raw=1`)** — `Content-Type: text/plain`

---

### POST /api/logs/clear

Truncates the server log file. A new INFO entry is written immediately after.

**Response**
```json
{ "status": "cleared", "path": "logs/metis.log" }
```

---

### POST /api/shutdown

Gracefully stops the server. Sends a 200 response then halts after 200 ms.

**Response**
```json
{ "status": "shutting_down" }
```
