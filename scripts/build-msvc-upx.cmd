@echo off
setlocal ENABLEDELAYEDEXPANSION

REM Build with MSVC first
call "%~dp0build-msvc.cmd"
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

set ROOT=%~dp0..\
pushd %ROOT%

set EXE=build-msvc\Release\c-hash.exe
if not exist "%EXE%" (
    echo Error: %EXE% not found.
    popd
    exit /b 1
)

where upx >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: upx not found in PATH. Install UPX and try again.
    popd
    exit /b 1
)

echo Compressing %EXE% with UPX...
upx --best --lzma "%EXE%"
if %ERRORLEVEL% NEQ 0 (
    echo Error: UPX compression failed.
    popd
    exit /b 1
)

for %%F in ("%EXE%") do set SIZE=%%~zF
echo Done. Compressed size: %SIZE% bytes

popd
endlocal


