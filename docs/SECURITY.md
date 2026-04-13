# Metis SEC EDGAR Fraud Analyzer - Security

**Version:** 3.1.3
**Author:** Bennie Shearer (Retired)

---

## Scope

This tool fetches only public data from SEC EDGAR. It does not store credentials,
personal data, or proprietary information.

## Hardening Recommendations

- Run on localhost or a trusted internal network only.
- Set `server.cors=false` in config.pson if browser access from other origins is
  not needed.
- Restrict the listening interface via `server.host` if multiple network interfaces
  are present.
- The SEC requires a valid email in `sec.user_agent`. Use a real address to avoid
  HTTP 403 errors and comply with SEC usage policies.

## Known Limitations

- No TLS/HTTPS support (stub reserved for future implementation).
- No authentication on REST endpoints.
- No request rate-limiting on incoming client connections.

## Reporting Issues

Contact the author via the repository issue tracker.
