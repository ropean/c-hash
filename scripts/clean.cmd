@echo off
setlocal

REM Change to repo root
set ROOT=%~dp0..\
pushd %ROOT%

set DIRS=
set DIRS=%DIRS% build
set DIRS=%DIRS% build-mingw
set DIRS=%DIRS% build-msvc
set DIRS=%DIRS% build-ninja-mingw
set DIRS=%DIRS% build-ninja-msvc

set FILES=
set FILES=%FILES% CMakeCache.txt
set FILES=%FILES% cmake_install.cmake
set FILES=%FILES% Makefile
set FILES=%FILES% build.ninja
set FILES=%FILES% .ninja_deps
set FILES=%FILES% .ninja_log
set FILES=%FILES% compile_commands.json
set FILES=%FILES% install_manifest.txt

set EXTRA_DIRS=
set EXTRA_DIRS=%EXTRA_DIRS% CMakeFiles
set EXTRA_DIRS=%EXTRA_DIRS% Testing
set EXTRA_DIRS=%EXTRA_DIRS% .vs

set VS_FILES=
set VS_FILES=%VS_FILES% *.sln
set VS_FILES=%VS_FILES% *.vcxproj
set VS_FILES=%VS_FILES% *.vcxproj.filters
set VS_FILES=%VS_FILES% *.vcxproj.user

echo Cleaning build directories...
for %%D in (%DIRS%) do (
    if exist "%%D" (
        echo - Removing directory: %%D
        rmdir /s /q "%%D"
    )
)

echo Cleaning CMake in-source files...
for %%F in (%FILES%) do (
    if exist "%%F" (
        echo - Deleting file: %%F
        del /f /q "%%F"
    )
)

for %%D in (%EXTRA_DIRS%) do (
    if exist "%%D" (
        echo - Removing directory: %%D
        rmdir /s /q "%%D"
    )
)

echo Cleaning Visual Studio project files...
for %%F in (%VS_FILES%) do (
    if exist "%%F" (
        echo - Deleting file: %%F
        del /f /q "%%F"
    )
)

echo Done.
popd
endlocal


