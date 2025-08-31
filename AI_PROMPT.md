## Project overview

This repository contains a Windows-only C++ application, c-hash, that computes SHA-256 hashes for files using a streamed approach. The primary deliverable is a minimal, fast, and user-friendly Win32 GUI.

- Platform: Windows 10/11
- Language/Standard: C++17
- UI Toolkit: Raw Win32 API
- Crypto API: Windows CNG (bcrypt)
- Build system: CMake
- Outputs: HEX (uppercase toggle), Base64, file size, elapsed, throughput
- Executable name: c-hash.exe (GUI only)

## Goals and non-goals

- Goal: Fast, streamed SHA-256 hashing for large files without blocking the UI.
- Goal: Simple, polished GUI with clear outputs and copy buttons.
- Non-goal: CLI tool (the CLI target was removed; only GUI is built).
- Non-goal: Cross-platform support (Windows-only at present).

## Repository structure

- `src/hash.hpp`, `src/hash.cpp`: Shared hashing utilities (streamed SHA-256 via CNG; HEX/Base64 encoding).
- `src/gui.cpp`: Win32 GUI application.
- `CMakeLists.txt`: CMake configuration for library and GUI target.
- `res/app.rc.in`: Resource template for icon and version metadata.
- `assets/app.ico`: Default icon (auto-detected if not overridden by APP_ICON env/cmake var).
- `scripts/`:
  - `build-mingw.cmd`: MinGW Makefiles build to `build-mingw`.
  - `build-msvc.cmd`: Visual Studio 2022 (MSVC) build to `build-msvc` (Release).
  - `build-ninja-mingw.cmd`: Ninja + MinGW build to `build-ninja-mingw` (Release).
  - `build-ninja-msvc.cmd`: Ninja Multi-Config + MSVC build to `build-ninja-msvc` (Release).
  - `build-msvc-upx.cmd`: MSVC build then compress `c-hash.exe` using UPX.
  - `build.cmd`: Visual Studio 2022 build to plain `build` directory (Release).

## Build requirements

- CMake 3.16+
- One of:
  - Visual Studio 2022 (MSVC) or Build Tools, Windows SDK
  - MinGW-w64 toolchain (x86_64)
  - Ninja (optional for Ninja-based scripts)
- UPX (optional; used by `build-msvc-upx.cmd`)

## Build and run

- MinGW Makefiles:

  ```bat
  scripts\build-mingw.cmd
  start build-mingw\c-hash.exe
  ```

- MSVC (Visual Studio 2022):

  ```bat
  scripts\build-msvc.cmd
  start build-msvc\Release\c-hash.exe
  ```

- Ninja + MinGW:

  ```bat
  scripts\build-ninja-mingw.cmd
  start build-ninja-mingw\c-hash.exe
  ```

- Ninja + MSVC:

  ```bat
  scripts\build-ninja-msvc.cmd
  start build-ninja-msvc\Release\c-hash.exe
  ```

- MSVC + UPX compression:
  ```bat
  scripts\build-msvc-upx.cmd
  ```

## Architecture

- `hashcore` (static lib):

  - `compute_sha256_streamed(path, digest, out_size, out_elapsed, out_error)`
    - Uses `CreateFileW`, `ReadFile` in 1 MiB chunks
    - Bcrypt: `BCryptOpenAlgorithmProvider(BCRYPT_SHA256_ALGORITHM)` → `BCryptCreateHash` → `BCryptHashData` → `BCryptFinishHash`
    - Measures elapsed using `QueryPerformanceCounter`
  - `to_hex(digest, uppercase: bool)`
  - `to_base64(digest)`

- GUI (`src/gui.cpp`):
  - Raw Win32 window with controls:
    - Path: `STATIC` label (width 80), `EDIT` box, Browse and Clear buttons
    - Uppercase HEX checkbox aligned to text boxes
    - Output rows with labels (width 80): HEX (readonly edit + Copy), Base64 (readonly edit + Copy), Size, Elapsed, Throughput
  - Drag & drop support (file)
  - Background hashing thread posts results back via `WM_APP + 1` (WM_HASH_DONE)
  - Busy state: disables interactive controls and sets title to `c-hash — Hashing...`
  - Transparent static labels, standard window background
  - Window centers on current monitor at startup
  - Sets small/big window icons and class icon from `IDI_ICON1`

## UX rules and behaviors

- Path selection (Browse or drag & drop) triggers hashing automatically.
- Uppercase HEX is a view option:
  - If no digest exists or Path is empty, do nothing
  - If digest exists, re-render HEX in place without recomputing
- While hashing (busy):
  - Disable inputs (path edit, browse, clear, uppercase toggle, output edits and copy buttons)
  - Title shows `c-hash — Hashing...`
  - Ignore drag & drop
- After hashing:
  - Re-enable controls and restore title to `c-hash`
- Units and formatting:
  - Size: choose GB/MB/bytes (decimal: 1e9/1e6), with thousands separators; up to 2 decimals for GB/MB
  - Elapsed: choose ms / s / m / h, with thousands separators; up to 2 decimals
  - Throughput: choose GB/s, MB/s, KB/s (decimal); up to 2 decimals
  - HEX and Base64 are computed from the raw SHA-256 digest
- Label widths (Path, HEX, Base64, Size, Elapsed, Throughput): 80 px, transparent background

## CMake details

- Targets:
  - `hashcore` (STATIC): links `bcrypt` on Windows
  - `c-hash-gui` (WIN32): produces `c-hash.exe`
- MSVC: compiled as UTF-8 (`/utf-8`) to avoid code page issues
- Resource embedding:
  - `res/app.rc.in` configured to `build/app.rc` (icon + VERSIONINFO)
  - Auto-detects `assets/app.ico` if `APP_ICON` is not provided
  - Adds version fields (FileVersion/ProductVersion) from project version

## Error handling and messaging

- File open/size read errors: show MessageBox with error message
- CNG failures (provider/hash/data/finish): show MessageBox
- ReadFile failures: show MessageBox
- Busy state always reset on completion (success or error)

## Coding standards

- C++17, prefer readable, explicit code.
- Keep functions single-responsibility and add guard clauses for errors.
- Avoid inline commentary; prefer short pre-line comments for non-obvious logic.
- Use descriptive names; avoid abbreviations.
- Follow existing formatting and indentation as in repository files.

## Extensibility guidance

- Additional algorithms (future):
  - SHA-1/MD5 via CNG are straightforward extensions
  - BLAKE3/xxHash require third-party libs; gate with CMake options like `WITH_BLAKE3`/`WITH_XXHASH`
  - Add a `Hasher` interface if multiple algorithms are introduced, then parameterize the GUI
- Multi-file support:
  - Consider a queue and a small thread pool (one worker per file) with result buffering for stable UI updates.
- Verification mode:
  - GUI button to compare computed hash to a provided reference value.
- Progress/UI polish:
  - Optional progress bar or spinner during hashing
  - ESC to cancel hashing (requires cooperative cancellation in the read loop)

## Testing guidance

- Functional:
  - Compare results with `Get-FileHash -Algorithm SHA256` or `certutil -hashfile <file> SHA256`
  - Validate Base64/HEX against known vectors
- Performance:
  - Hash large files; ensure UI remains responsive and busy state is correctly toggled
  - Check throughput and elapsed display sanity
- UX:
  - Verify labels align; transparent backgrounds; window centers on launch
  - Uppercase toggle does not recompute
  - Drag & drop blocked while busy

## Operational notes

- If a build fails with `Permission denied` on `c-hash.exe`, close the running app and rebuild.
- To embed a custom icon:
  ```bat
  set APP_ICON=assets\app.ico
  scripts\build-msvc.cmd
  ```

## Known constraints

- Windows-only due to the use of Win32 and CNG
- Single-file hashing per operation in the GUI (at present)

## Quick facts (for AI assistants)

- GUI entry point: `wWinMain` in `src/gui.cpp`
- Hashing API: `hashcore::compute_sha256_streamed` in `src/hash.cpp`
- Busy state: `SetUiBusy`, `WM_HASH_DONE` processing in `WndProc`
- Icon resource symbol: `IDI_ICON1` from `res/app.rc.in`
- Executable name: `c-hash.exe`
