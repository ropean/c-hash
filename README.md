# c-hash

Windows C++ GUI hasher, computes SHA-256 via Windows CNG with streamed I/O and a minimal Win32 UI.

Features

- Streamed SHA-256 hashing (1 MiB buffer)
- HEX output (uppercase toggle), Base64 output of raw digest
- Size, elapsed, throughput with human-friendly units and thousands separators
- Drag & drop, Browse, Copy buttons, Uppercase toggle alignment
- Non-blocking background hashing with UI busy state and title tip
- Centered startup window, embedded icon and version info

Build

- MSVC (Visual Studio 2022):

  ```bat
  scripts\build-msvc.cmd
  start build-msvc\Release\c-hash.exe
  ```

- MinGW (Makefiles):

  ```bat
  scripts\build-mingw.cmd
  start build-mingw\c-hash.exe
  ```

- Ninja + MSVC:

  ```bat
  scripts\build-ninja-msvc.cmd
  start build-ninja-msvc\Release\c-hash.exe
  ```

- Ninja + MinGW:

  ```bat
  scripts\build-ninja-mingw.cmd
  start build-ninja-mingw\c-hash.exe
  ```

- MSVC + UPX compression:
  ```bat
  scripts\build-msvc-upx.cmd
  ```

Configuration

- Optional icon embedding:
  - Provide an `.ico` via CMake cache var `APP_ICON` or place `assets\app.ico` in the repo; the build will embed it.

Notes

- Windows-only; uses Windows CNG (`bcrypt`) and raw Win32 APIs.
- Throughput is approximate (based on file size and wall-clock).

For a deep, structured overview (architecture, UX, extensibility), see `AI_PROMPT.md`.
