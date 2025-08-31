@echo off
setlocal ENABLEDELAYEDEXPANSION

REM Configure paths
set ROOT=%~dp0..\
pushd %ROOT%

set BUILD_DIR=build-ninja-mingw
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM Configure with Ninja + MinGW (requires ninja and gcc/g++ in PATH)
cmake -S . -B %BUILD_DIR% -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

cmake --build %BUILD_DIR%
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo.
echo Built: %BUILD_DIR%\c-hash.exe
popd
endlocal


