# Metis SEC EDGAR Fraud Analyzer - Build Instructions

**Version:** 3.1.3
**Author:** Bennie Shearer (Retired)

---

## Requirements

| Platform | Compiler | CMake |
|----------|----------|-------|
| Windows  | MinGW-w64 GCC 13+ | 3.16+ |
| Linux    | GCC 13+ or Clang 16+ | 3.16+ |
| macOS    | Apple Clang 15+ | 3.16+ |

No external libraries required. Windows links `ws2_32` and `winhttp`.
Linux/macOS use system `curl` for HTTPS.

---

## Windows (CLion / MinGW-w64)

```bash
mkdir cmake-build-release
cd cmake-build-release
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j4
```

Executable: `cmake-build-release\bin\metis-sec-edgar-fraud-analyzer.exe`
Client:     `cmake-build-release\bin\client\`

---

## Linux / macOS

```bash
mkdir cmake-build-release && cd cmake-build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Executable: `cmake-build-release/bin/metis-sec-edgar-fraud-analyzer`
Client:     `cmake-build-release/bin/client/`

---

## Running

Copy `config.pson` to the same directory as the executable, then:

```bash
./metis-sec-edgar-fraud-analyzer
# or with explicit config path:
./metis-sec-edgar-fraud-analyzer --config /path/to/config.pson
```

Open browser at `http://localhost:8080` (or the port set in config.pson).

---

## Version Bumping

1. Edit `VERSION` file (single line: `MAJOR.MINOR.PATCH`)
2. Re-run CMake configure: `cmake ..` (regenerates `version.h`)
3. Update `docs/CHANGELOG.md`
4. Verify all references: search for old version string in all files

---

## Packaging

```bash
cd cmake-build-release
zip -r metis-sec-edgar-fraud-analyzer-3.1.3.zip bin/ ../config.pson ../docs/
```
