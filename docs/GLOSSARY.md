# SEC EDGAR Fraud Analyzer - Glossary

**Version:** 3.1.3  
**Author:** Bennie Shearer (Retired)

**DISCLAIMER: This project is NOT funded, endorsed, or approved by the U.S. Securities and Exchange Commission (SEC).**

---

## Financial Terms

### Accruals
Non-cash accounting entries that recognize revenue or expenses before cash changes hands. High accruals relative to cash flow can indicate earnings manipulation.

### Altman Z-Score
A formula developed by Edward Altman in 1968 to predict bankruptcy risk using five financial ratios. Scores below 1.81 indicate high distress.

### Asset Quality
Measure of the proportion of assets that are tangible vs. intangible. Declining asset quality may indicate manipulation.

### Benford's Law
Mathematical law stating that leading digits in naturally occurring datasets follow a specific distribution (1 appears ~30% of the time). Deviation suggests artificial data.

### Beneish M-Score
An eight-variable model developed by Messod Beneish to detect earnings manipulation. Scores above -1.78 suggest likely manipulation.

### Cash Flow from Operations (CFO)
Cash generated from normal business activities, excluding investing and financing activities.

### CIK (Central Index Key)
A unique 10-digit identifier assigned by the SEC to companies and individuals filing with the SEC.

### Current Ratio
Current Assets divided by Current Liabilities. Measures short-term liquidity.

### EBIT
Earnings Before Interest and Taxes. A measure of operating profitability.

### Fraud Triangle
Framework by Donald Cressey identifying three conditions for fraud: Pressure (incentive), Opportunity, and Rationalization.

### Gross Margin
(Revenue - Cost of Goods Sold) / Revenue. Declining margins may indicate financial stress.

### Leverage
Use of debt to finance assets. High leverage increases financial risk.

### Market Value of Equity
Share price multiplied by shares outstanding. Used in Altman Z-Score.

### Net Income
Total profit after all expenses, taxes, and costs. Also called "bottom line."

### Piotroski F-Score
A nine-point scoring system developed by Joseph Piotroski to evaluate financial strength. Scores 0-3 indicate weak fundamentals.

### Retained Earnings
Cumulative profits kept in the company rather than distributed as dividends.

### Return on Assets (ROA)
Net Income divided by Total Assets. Measures how efficiently assets generate profit.

### Revenue
Total income from sales of goods or services. Also called "top line" or "sales."

### Working Capital
Current Assets minus Current Liabilities. Measures short-term financial health.

---

## Technical Terms

### API (Application Programming Interface)
A set of protocols for building and interacting with software. This tool provides a REST API for programmatic access.

### Cache
Temporary storage of data to speed up subsequent requests. Reduces load on SEC servers.

### C++20
The 2020 version of the C++ programming language standard used for the server component.

### CORS (Cross-Origin Resource Sharing)
Browser security feature that restricts web pages from making requests to different domains.

### Endpoint
A specific URL path in an API that performs a particular function (e.g., /api/analyze).

### HTTP
Hypertext Transfer Protocol. The foundation of data communication on the web.

### JSON (JavaScript Object Notation)
A lightweight data format used for API responses and configuration files.

### Rate Limiting
Restricting the number of requests a user can make in a given time period. SEC limits to 10 requests/second.

### REST (Representational State Transfer)
An architectural style for designing networked applications using HTTP methods.

### User-Agent
HTTP header identifying the client making a request. SEC requires this for API access.

### XBRL (eXtensible Business Reporting Language)
Standardized format for financial data that enables automated analysis of SEC filings.

---

## SEC Filing Types

### 10-K
Annual report containing audited financial statements, business description, and risk factors. Most comprehensive filing.

### 10-Q
Quarterly report with unaudited financial statements. Filed three times per year (Q1, Q2, Q3).

### 8-K
Current report for material events (acquisitions, executive changes, etc.). Filed within 4 business days of event.

### DEF 14A
Definitive proxy statement with executive compensation, board information, and shareholder proposals.

### Form 4
Insider trading report. Must be filed within 2 business days of a trade by officers, directors, or 10% owners.

---

## Risk Levels

### LOW (0-25)
Minimal fraud indicators. Company appears financially healthy with normal patterns.

### MODERATE (26-50)
Some elevated indicators. Warrants closer examination of specific areas.

### HIGH (51-75)
Significant red flags across multiple models. Requires thorough investigation.

### CRITICAL (76-100)
Strong fraud indicators. Multiple models showing concerning patterns.

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
