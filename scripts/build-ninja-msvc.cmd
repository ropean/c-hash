@echo off
setlocal ENABLEDELAYEDEXPANSION

REM Configure paths
set ROOT=%~dp0..\
pushd %ROOT%

set BUILD_DIR=build-ninja-msvc
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM Configure with Ninja + MSVC (requires Visual Studio Build Tools and ninja)
cmake -S . -B %BUILD_DIR% -G "Ninja Multi-Config"
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

REM Build Release configuration
cmake --build %BUILD_DIR% --config Release
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo.
echo Built: %BUILD_DIR%\Release\c-hash.exe
popd
endlocal


