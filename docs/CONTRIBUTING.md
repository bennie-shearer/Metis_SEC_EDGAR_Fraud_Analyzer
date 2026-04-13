# Metis SEC EDGAR Fraud Analyzer - Contributing

**Version:** 3.1.3
**Author:** Bennie Shearer (Retired)

---

## Conventions

- C++20, zero external dependencies (no Boost, no GTest, no Doxygen).
- All `.md` and `.txt` files in `docs/`.
- CMake project name: `Metis_SEC_EDGAR_Fraud_Analyzer` (underscores).
- Binary and zip names: `metis-sec-edgar-fraud-analyzer` (hyphens).
- Version: `MAJOR.MINOR.PATCH` with dots (never underscores in version strings).
- MIT License header in every `.cpp` and `.h` file.
- American English throughout.

## Version Bumping

1. Edit `VERSION` (the only file that needs a manual change).
2. Re-run `cmake ..` to regenerate `include/sec_analyzer/version.h`.
3. Update `docs/CHANGELOG.md` with a new `[X.Y.Z]` entry.
4. Verify all version references match: grep for the old version string.

## C++ Files

- Edit `.cpp` and `.h` files only with `str_replace` or `create_file` — never with
  bulk `sed` that could corrupt historical `CHANGELOG` entries.
- Brace-balance checking may be done with Python scripts.
- No Docker, Kubernetes manifests, Doxygen config, GTest, PyTest, or Jupyter.

## Submitting Changes

Open a pull request with:
- Updated `VERSION` file
- Updated `docs/CHANGELOG.md`
- All version references consistent
