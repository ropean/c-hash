@echo off
setlocal ENABLEDELAYEDEXPANSION

REM Configure paths
set ROOT=%~dp0..\
pushd %ROOT%

set BUILD_DIR=build-mingw
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM Detect mingw32-make or make
where mingw32-make >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    set MAKE=mingw32-make
) else (
    where make >nul 2>nul
    if %ERRORLEVEL% EQU 0 (
        set MAKE=make
    ) else (
        echo Error: mingw32-make/make not found in PATH.
        exit /b 1
    )
)

REM Configure with MinGW Makefiles. CMake will auto-detect icon from assets.
cmake -S . -B %BUILD_DIR% -G "MinGW Makefiles"
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

%MAKE% -C %BUILD_DIR%
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo.
echo Built: %BUILD_DIR%\c-hash.exe
popd
endlocal

