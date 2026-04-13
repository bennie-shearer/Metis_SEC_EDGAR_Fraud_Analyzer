# Metis SEC EDGAR Fraud Analyzer - Troubleshooting

**Version:** 3.1.3
**Author:** Bennie Shearer (Retired)

---

## Server Won't Start

**Port already in use**
Change `server.port` in `config.pson` to an unused port, or stop the process
using the current port.

**Failed to bind to port**
On Linux/macOS, ports below 1024 require root. Use a port >= 1024.

---

## HTTP 403 from SEC EDGAR

SEC requires a User-Agent in the exact format: `FirstName LastName email@domain.com`

Do **not** use parentheses, app names, or version strings — these cause 403.

Edit `config.pson`:
```
sec.user_agent=Bob Doe bob@doe.com
```

---

## HTTP 429 from SEC EDGAR (Rate Limited)

Increase the delay between requests:
```
sec.rate_limit_ms=500
```

---

## "Company not found"

- Verify the ticker symbol is correct and the company is still listed.
- For delisted companies, use the CIK number instead (Tools > CIK Lookup).
- Some foreign companies use the 20-F form and may have limited XBRL data.

---

## Analysis Shows Zero / Missing Values

- The company may not file XBRL-tagged financial data (common for older filings
  and smaller companies before 2009).
- Try increasing `analysis.default_years` in config.pson to find more filings.
- Check `logs/metis.log` for XBRL extraction warnings.

---

## Client Shows "Not Connected"

1. Confirm the server is running (`metis-sec-edgar-fraud-analyzer`).
2. Open **Connect > Connection Settings**.
3. Set the API URL to `http://localhost:8080` (or the configured port).
4. Click **Test Connection**.
5. Alternatively, enable **Demo Mode** (Ctrl+M) to test the interface offline.

---

## Windows: DLL Errors at Runtime

Build with static linking enabled or ensure the MinGW-w64 runtime DLLs
(`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`) are in the
same directory as the executable or on the system PATH.

---

## Linux: `curl` Not Found

The Linux/macOS SEC fetcher uses the system `curl` command. Install it:

```bash
# Debian/Ubuntu
sudo apt-get install curl

# Fedora/RHEL
sudo dnf install curl

# macOS
brew install curl
```

---

## Log File Not Created

Ensure the `logs/` directory exists relative to the working directory, or
set an absolute path in config.pson:
```
paths.log_file=/var/log/metis/metis.log
```

The server creates the directory automatically if it can be inferred from the path.
