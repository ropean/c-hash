@echo off
setlocal ENABLEDELAYEDEXPANSION

REM Configure paths
set ROOT=%~dp0..\
pushd %ROOT%

set BUILD_DIR=build-msvc
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM Generate Visual Studio 2022 (x64)
cmake -S . -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

REM Build Release configuration
cmake --build %BUILD_DIR% --config Release
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo.
echo Built: %BUILD_DIR%\Release\c-hash.exe
popd
endlocal


