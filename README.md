# c-hash

Minimal Windows C++ CLI hasher mirroring the logic of `rust-hash`.

Features

- Streamed SHA-256 hashing (1 MiB buffer)
- HEX output with optional uppercase `-u`
- Base64 output of raw digest
- Shows file size, elapsed time, and approximate throughput

Build (CMake + MSVC)

```bat
cd c-hash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Run

```bat
build\Release\c-hash.exe [-u] <file_path>
```

Example

```bat
build\Release\c-hash.exe -u C:\\Windows\\notepad.exe
```

Notes

- Uses Windows CNG (`bcrypt`) for SHA-256; no external dependencies.
- Throughput uses file size and wall-clock; it is approximate.

MinGW-w64 (WinLibs) Build

```bat
scripts\build-mingw.cmd
```

- The script attempts to copy the icon from `rust-hash` (via `.cargo/config.toml` `APP_ICON` if present),
  falls back to `assets\app.ico` if found, and embeds it automatically.
- Output: `build-mingw\c-hash.exe`
