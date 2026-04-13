# SEC EDGAR Fraud Analyzer - Examples

**Version:** 3.1.3  
**Author:** Bennie Shearer (Retired)

**DISCLAIMER: This project is NOT funded, endorsed, or approved by the U.S. Securities and Exchange Commission (SEC).**

---

## Table of Contents

1. [Basic Analysis](#1-basic-analysis)
2. [Batch Analysis](#2-batch-analysis)
3. [API Examples](#3-api-examples)
4. [Interpreting Results](#4-interpreting-results)
5. [Command Line Examples](#5-command-line-examples)

---

## 1. Basic Analysis

### Analyzing Apple Inc.

1. Start the server: `./sec_fraud_analyzer`
2. Open web interface: `http://localhost:8080`
3. Enter ticker: `AAPL`
4. Click "Analyze"

**Expected Output:**

```
Company: Apple Inc.
Ticker: AAPL
CIK: 0000320193

Overall Risk: LOW (Score: 18.5)

Model Results:
- Beneish M-Score: -2.85 (LOW - Unlikely manipulation)
- Altman Z-Score: 4.92 (LOW - Safe zone)
- Piotroski F-Score: 8/9 (LOW - Strong financials)
- Benford's Law: 3.2% deviation (LOW - Normal)
- Fraud Triangle: 12% (LOW - Minimal pressure/opportunity)
```

### Analyzing a High-Risk Company

Some companies may show elevated risk scores:

```
Company: Example Corp
Ticker: EXMP

Overall Risk: HIGH (Score: 67.3)

Model Results:
- Beneish M-Score: -1.45 (HIGH - Possible manipulation)
- Altman Z-Score: 1.52 (HIGH - Distress zone)
- Piotroski F-Score: 3/9 (HIGH - Weak financials)
- Benford's Law: 18.5% deviation (MODERATE)
- Fraud Triangle: 58% (HIGH - Multiple red flags)
```

---

## 2. Batch Analysis

### Analyzing Multiple Companies

1. Click Tools > Batch Analysis
2. Enter tickers (one per line):

```
AAPL
MSFT
GOOGL
AMZN
META
```

3. Click "Analyze All"

**Output Summary Table:**

| Ticker | Company | Risk Level | Score |
|--------|---------|------------|-------|
| AAPL | Apple Inc. | LOW | 18.5 |
| MSFT | Microsoft Corp | LOW | 22.1 |
| GOOGL | Alphabet Inc. | LOW | 19.8 |
| AMZN | Amazon.com Inc. | MODERATE | 34.2 |
| META | Meta Platforms | LOW | 25.7 |

### Comparing Peer Companies

Analyze competitors in the same industry:

```
# Retail Banking
JPM
BAC
WFC
C
USB
```

---

## 3. API Examples

### Health Check

```bash
curl http://localhost:8080/api/health
```

Response:
```json
{
  "status": "ok",
  "version": "3.1.3",
  "uptime_seconds": 3600
}
```

### Single Company Analysis

```bash
curl "http://localhost:8080/api/analyze?ticker=AAPL&years=5"
```

Response:
```json
{
  "status": "success",
  "data": {
    "company": {
      "name": "Apple Inc.",
      "ticker": "AAPL",
      "cik": "0000320193"
    },
    "overall_risk": {
      "level": "LOW",
      "score": 18.5
    },
    "models": {
      "beneish": {
        "m_score": -2.85,
        "risk_level": "LOW",
        "interpretation": "Unlikely manipulation"
      },
      "altman": {
        "z_score": 4.92,
        "risk_level": "LOW",
        "zone": "Safe"
      },
      "piotroski": {
        "f_score": 8,
        "risk_level": "LOW",
        "rating": "Strong"
      }
    },
    "filings_analyzed": 17
  }
}
```

### Company Search

```bash
curl "http://localhost:8080/api/search?q=microsoft"
```

Response:
```json
{
  "status": "success",
  "data": {
    "results": [
      {"ticker": "MSFT", "name": "Microsoft Corporation", "cik": "0000789019"}
    ]
  }
}
```

---

## 4. Interpreting Results

### Risk Level Guidelines

| Level | Score | Action |
|-------|-------|--------|
| LOW | 0-25 | Normal - no immediate concerns |
| MODERATE | 26-50 | Review - investigate specific models |
| HIGH | 51-75 | Caution - significant red flags |
| CRITICAL | 76-100 | Alert - strong fraud indicators |

### Model-Specific Interpretation

**Beneish M-Score:**
- M > -1.78: Company likely manipulating earnings
- M < -2.22: Company unlikely to be manipulating
- Between: Grey zone, requires judgment

**Altman Z-Score:**
- Z > 2.99: Safe zone - financially healthy
- Z 1.81-2.99: Grey zone - uncertain
- Z < 1.81: Distress zone - bankruptcy risk

**Piotroski F-Score:**
- 8-9: Strong financial health
- 5-7: Average
- 0-4: Weak - potential problems

---

## 5. Command Line Examples

### Start Server on Custom Port

```bash
./sec_fraud_analyzer --port 3000
```

### Enable Debug Logging

```bash
./sec_fraud_analyzer --log-level debug
```

### Log to File

```bash
./sec_fraud_analyzer --log-file analyzer.log
```

### Quiet Mode (No Console Output)

```bash
./sec_fraud_analyzer --quiet --log-file analyzer.log
```

### Full Example

```bash
./sec_fraud_analyzer --port 8080 --log-level info --log-file logs/analyzer.log
```

### Check Version

```bash
./sec_fraud_analyzer --version
# Output: SEC EDGAR Fraud Analyzer v3.1.3
```

### Show Help

```bash
./sec_fraud_analyzer --help
# Output: Usage and options
```

---

## Limitations

1. **Data Availability**: Analysis depends on SEC EDGAR data availability
2. **XBRL Coverage**: Not all filings have complete XBRL data
3. **Industry Variations**: Model thresholds may vary by industry
4. **Historical Data**: Older filings may lack standardized data

---

## Acknowledgments

- **Walter Hamscher** - Mentor and XBRL expert
- **SEC EDGAR** - Financial data source
- **Academic researchers** - Beneish, Altman, Piotroski, Cressey, Benford
- **CLion** by JetBrains s.r.o.
- **Claude** by Anthropic PBC

---

**Version:** 3.1.3 | **Date:** January 2026
