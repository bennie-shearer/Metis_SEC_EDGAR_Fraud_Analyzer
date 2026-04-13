# SEC EDGAR Fraud Analyzer - Frequently Asked Questions

**Version:** 3.1.3  
**Author:** Bennie Shearer (Retired)

**DISCLAIMER: This project is NOT funded, endorsed, or approved by the U.S. Securities and Exchange Commission (SEC).**

---

## Table of Contents

1. [General Questions](#1-general-questions)
2. [Data and Analysis](#2-data-and-analysis)
3. [Technical Issues](#3-technical-issues)
4. [Results Interpretation](#4-results-interpretation)
5. [Legal and Compliance](#5-legal-and-compliance)

---

## 1. General Questions

### Q: What is the SEC EDGAR Fraud Analyzer?

A: An educational tool that analyzes SEC filings using five academically-validated fraud detection models: Beneish M-Score, Altman Z-Score, Piotroski F-Score, Benford's Law, and Fraud Triangle analysis.

### Q: Who should use this tool?

A: Students, researchers, educators, and investors seeking to understand fraud detection methodologies. It is NOT intended as the sole basis for investment decisions.

### Q: Is this tool free to use?

A: Yes, it is open source under the MIT License for educational and research purposes.

### Q: What companies can I analyze?

A: Any US publicly traded company that files with the SEC. This includes companies listed on NYSE, NASDAQ, and other US exchanges.

---

## 2. Data and Analysis

### Q: Where does the data come from?

A: All data is sourced from the SEC EDGAR database, which is publicly available and free to access.

### Q: How current is the data?

A: Data is fetched in real-time from SEC EDGAR. New filings typically appear within 24-48 hours of submission.

### Q: Why is revenue showing $0?

A: Some companies use non-standard XBRL concepts for revenue. The system tries multiple concept names, but some may not be captured.

### Q: Why is a company not found?

A: Possible reasons:
- Company is delisted
- Company is private (not SEC filer)
- Wrong ticker format (try BRK-A instead of BRK.A)
- Company recently changed its ticker

### Q: How many years of data are analyzed?

A: By default, 5 years. You can adjust from 1-10 years in the settings.

### Q: What filing types are analyzed?

A: 10-K (annual reports), 10-Q (quarterly reports), and their amendments.

---

## 3. Technical Issues

### Q: Why is analysis slow?

A: The SEC limits API requests to 10 per second. First-time analysis requires fetching data from SEC servers. Subsequent analyses use cached data and are faster.

### Q: Can I run this offline?

A: No, the tool requires internet access to fetch data from SEC EDGAR.

### Q: What browsers are supported?

A: Chrome 90+, Firefox 88+, Edge 90+, Safari 14+

### Q: Why won't the server start?

A: Common causes:
- Port 8080 already in use (try --port 3000)
- Missing permissions
- Firewall blocking connections

### Q: How do I clear the cache?

A: Use Tools > Clear Cache in the web interface, or restart the server.

---

## 4. Results Interpretation

### Q: Does a high risk score mean the company is committing fraud?

A: NO. A high score indicates elevated risk factors that warrant further investigation. False positives are common. These are statistical indicators, not proof of fraud.

### Q: What should I do with a HIGH or CRITICAL result?

A: Consider it a starting point for deeper analysis:
1. Review the specific model scores
2. Read the actual SEC filings
3. Compare with industry peers
4. Consult professional analysts
5. Do not make investment decisions based solely on this tool

### Q: Why do different models give different results?

A: Each model measures different aspects:
- Beneish: Earnings manipulation patterns
- Altman: Bankruptcy/distress risk
- Piotroski: Financial strength
- Benford: Number pattern anomalies
- Fraud Triangle: Behavioral factors

### Q: How accurate are the models?

A: Academic studies show:
- Beneish M-Score: ~76% accuracy in identifying manipulators
- Altman Z-Score: ~80-90% accuracy for bankruptcy prediction
- Note: Past performance does not guarantee future results

---

## 5. Legal and Compliance

### Q: Is using this tool legal?

A: Yes. All data is publicly available from SEC EDGAR, which is designed for public access.

### Q: Does this tool access any private data?

A: No. Only publicly filed SEC documents are accessed.

### Q: Can I use this for investment decisions?

A: This tool is for educational purposes only. Always consult qualified financial professionals before making investment decisions.

### Q: Is there any liability for using this tool?

A: The software is provided "as is" without warranty. Users assume all responsibility for how they use the results. See LICENSE.txt for full terms.

### Q: Does this tool comply with SEC API guidelines?

A: Yes. The tool:
- Includes proper User-Agent headers
- Respects rate limits (10 requests/second)
- Uses only public endpoints
- Does not scrape or circumvent restrictions

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
