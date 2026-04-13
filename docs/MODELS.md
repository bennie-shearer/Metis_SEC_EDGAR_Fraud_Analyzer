# Metis SEC EDGAR Fraud Analyzer - Fraud Detection Models

**Version:** 3.1.3
**Author:** Bennie Shearer (Retired)

**DISCLAIMER: This project is NOT funded, endorsed, or approved by the U.S. Securities and Exchange Commission (SEC).**

---

## Overview

The analyzer applies five established academic models, then combines them into a
composite risk score using configurable weights (set in `config.pson`).

```
Composite Score = Σ(model_weight × model_risk_score)
```

Default weights: Beneish 30%, Altman 25%, Piotroski 15%, Fraud Triangle 15%,
Benford 5%, Red Flags 10%.

---

## Beneish M-Score

**Reference:** Beneish, M.D. (1999) *The Detection of Earnings Manipulation*

Detects potential earnings manipulation using eight financial ratios.

```
M = -4.84 + 0.920·DSRI + 0.528·GMI + 0.404·AQI + 0.892·SGI
          + 0.115·DEPI - 0.172·SGAI + 4.679·TATA - 0.327·LVGI
```

| Component | Description |
|-----------|-------------|
| DSRI | Days Sales in Receivables Index |
| GMI  | Gross Margin Index |
| AQI  | Asset Quality Index |
| SGI  | Sales Growth Index |
| DEPI | Depreciation Index |
| SGAI | SG&A Expense Index |
| LVGI | Leverage Index |
| TATA | Total Accruals to Total Assets |

**Interpretation:**
- M > -2.22: Likely manipulator (high risk)
- -2.50 < M ≤ -2.22: Elevated risk
- M ≤ -2.50: Unlikely manipulator (low risk)

---

## Altman Z-Score

**Reference:** Altman, E.I. (1968) *Financial Ratios, Discriminant Analysis and the Prediction of Corporate Bankruptcy*

Predicts bankruptcy probability using five financial ratios.

```
Z = 1.2·X1 + 1.4·X2 + 3.3·X3 + 0.6·X4 + 1.0·X5
```

| Component | Formula |
|-----------|---------|
| X1 | Working Capital / Total Assets |
| X2 | Retained Earnings / Total Assets |
| X3 | EBIT / Total Assets |
| X4 | Market (or Book) Value of Equity / Total Liabilities |
| X5 | Sales / Total Assets |

**Zones:**
- Z > 2.99: Safe Zone
- 1.81 < Z ≤ 2.99: Gray Zone (uncertain)
- Z ≤ 1.81: Distress Zone (high bankruptcy probability)

For non-manufacturing companies, the Z''-Score model is also available
(`AltmanZPrimeModel`) using different coefficients and thresholds.

---

## Piotroski F-Score

**Reference:** Piotroski, J.D. (2000) *Value Investing: The Use of Historical Financial Statement Information to Separate Winners from Losers*

A 9-point financial strength scoring system.

**Profitability (4 points)**
1. ROA > 0
2. Operating Cash Flow > 0
3. ROA increasing year-over-year
4. CFO > Net Income (quality of earnings)

**Leverage & Liquidity (3 points)**
5. Leverage decreasing
6. Current ratio increasing
7. No new share issuance (no dilution)

**Operating Efficiency (2 points)**
8. Gross margin increasing
9. Asset turnover increasing

**Interpretation:**
- 0–3: Weak (potential short candidate)
- 4–6: Moderate
- 7–9: Strong (potential long candidate)

---

## Fraud Triangle

**Reference:** Cressey, D.R. (1953) *Other People's Money*

Assesses three conditions commonly present when fraud occurs.

**Pressure (35% weight)**
- Declining revenue trend
- Declining profit margins
- High leverage ratio (debt > 60% of assets)
- Negative operating cash flow
- Pattern of barely meeting earnings targets

**Opportunity (35% weight)**
- Complex organizational structure (high intangibles/goodwill ratio > 30%)
- Unusual changes in receivables or inventory (> 50% spike)
- Significant changes in accounting estimates (depreciation volatility > 30%)

**Rationalization (30% weight)**
- Aggressive accounting (net income > 150% of operating cash flow)
- Earnings consistently at boundary levels (margin 0–1% repeatedly)

**Composite risk:**
```
Overall = 0.35·Pressure + 0.35·Opportunity + 0.30·Rationalization
```

Risk levels: LOW (< 0.20), ELEVATED (0.20–0.40), MODERATE (0.40–0.70), HIGH (≥ 0.70)

---

## Benford's Law

**Reference:** Benford, F. (1938) *The Law of Anomalous Numbers*

Tests whether the leading digit distribution of financial figures conforms to
Benford's expected distribution. Significant deviation may indicate manipulation.

**Expected first-digit distribution:**

| Digit | Expected |
|-------|---------|
| 1 | 30.1% |
| 2 | 17.6% |
| 3 | 12.5% |
| 4 | 9.7% |
| 5 | 7.9% |
| 6 | 6.7% |
| 7 | 5.8% |
| 8 | 5.1% |
| 9 | 4.6% |

**Tests applied:**
- Chi-Square goodness-of-fit test
- Mean Absolute Deviation (MAD) — Nigrini conformity guidelines:
  - MAD ≤ 0.006: Close conformity
  - MAD ≤ 0.012: Acceptable conformity
  - MAD ≤ 0.015: Marginally acceptable
  - MAD > 0.015: Nonconformity (suspicious)
- Z-test per digit (p < 0.01 threshold)

A second-digit model (`BenfordSecondDigitModel`) is also implemented.

---

## Limitations

1. **Data Availability:** Analysis depends on SEC EDGAR XBRL data completeness.
   Older filings may lack standardized XBRL tags.
2. **XBRL Coverage:** Not all companies file complete XBRL-tagged financial data.
3. **Industry Variations:** Model thresholds were calibrated for manufacturing and
   public companies. Results may differ for financial, utility, or foreign companies.
4. **Historical Data:** Filings before 2009 may have limited XBRL coverage.
5. **Market Cap:** Altman X4 uses book equity when market cap is unavailable.
6. **Benford Validity:** Requires a sufficient number of financial data points;
   results are less reliable with fewer than 30 values.
