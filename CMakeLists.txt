cmake_minimum_required(VERSION 3.16)

# Derive version from tag name (GitHub Actions env or local git), fallback to 0.1.0
set(PROJECT_SEMVER "0.1.0")

# Prefer GitHub Actions tag env if present
if(DEFINED ENV{GITHUB_REF_NAME} AND NOT "$ENV{GITHUB_REF_NAME}" STREQUAL "")
    set(_tag "$ENV{GITHUB_REF_NAME}")
elseif(DEFINED ENV{GITHUB_REF} AND NOT "$ENV{GITHUB_REF}" STREQUAL "")
    set(_tag "$ENV{GITHUB_REF}")
endif()
if(DEFINED _tag)
    string(REGEX REPLACE "^refs/tags/" "" _tag "${_tag}")
    string(REGEX REPLACE "^v" "" _tag "${_tag}")
    if(_tag MATCHES "^[0-9]+\\.[0-9]+(\\.[0-9]+)?$")
        set(PROJECT_SEMVER "${_tag}")
    endif()
endif()

# If still default, try git describe --tags
if(PROJECT_SEMVER STREQUAL "0.1.0")
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --tags --abbrev=0
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(GIT_TAG)
            string(REGEX REPLACE "^v" "" GIT_TAG "${GIT_TAG}")
            if(GIT_TAG MATCHES "^[0-9]+\\.[0-9]+(\\.[0-9]+)?$")
                set(PROJECT_SEMVER "${GIT_TAG}")
            endif()
        endif()
    endif()
endif()

project(c-hash VERSION ${PROJECT_SEMVER} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Treat sources as UTF-8 on MSVC to avoid code page issues
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_library(hashcore STATIC
    src/hash.cpp
    src/hash.hpp
)

## CLI target removed per request

if(WIN32)
    # Link bcrypt on Windows (name is lowercase for MinGW)
    target_link_libraries(hashcore PUBLIC bcrypt)
endif()

target_compile_definitions(hashcore PUBLIC NOMINMAX)

# For MinGW, enable wide-char entry point (GUI target handled below)

# GUI target (Win32)
if(WIN32)
    add_executable(c-hash-gui WIN32
        src/gui.cpp
    )
    target_link_libraries(c-hash-gui PRIVATE hashcore user32 gdi32 comdlg32 shell32)
    target_compile_definitions(c-hash-gui PRIVATE NOMINMAX)
    set_target_properties(c-hash-gui PROPERTIES OUTPUT_NAME "c-hash")
endif()

# Compute version components for resource script
set(PROJECT_VERSION_STRING "${PROJECT_VERSION}")
string(REPLACE "." ";" VERSION_LIST "${PROJECT_VERSION_STRING}")
list(LENGTH VERSION_LIST _ver_len)
if(_ver_len GREATER 0)
    list(GET VERSION_LIST 0 VERSION_MAJOR)
else()
    set(VERSION_MAJOR 0)
endif()
if(_ver_len GREATER 1)
    list(GET VERSION_LIST 1 VERSION_MINOR)
else()
    set(VERSION_MINOR 0)
endif()

# Optional Windows icon embedding via APP_ICON variable
set(APP_ICON "${APP_ICON}" CACHE FILEPATH "Path to a .ico file to embed")

# Auto-detect icon in assets if APP_ICON not provided
if(WIN32 AND NOT APP_ICON)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.ico")
        set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.ico")
    endif()
endif()

if(WIN32 AND APP_ICON AND EXISTS "${APP_ICON}")
    set(RC_OUT "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/res/app.rc.in
        ${RC_OUT}
        @ONLY
    )
    if(TARGET c-hash-gui)
        target_sources(c-hash-gui PRIVATE ${RC_OUT})
    endif()
endif()

# Provide application title with version to GUI target
if(TARGET c-hash-gui)
    target_compile_definitions(c-hash-gui PRIVATE APP_TITLE_W=L"c-hash v${PROJECT_VERSION_STRING}")
endif()

# Pass version info to resource
set(PROJECT_VERSION "${PROJECT_VERSION}")
string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/app.rc.in ${CMAKE_CURRENT_BINARY_DIR}/app.rc @ONLY)

# For MinGW, also ensure GUI target uses wide-char entry
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(TARGET c-hash-gui)
        target_link_options(c-hash-gui PRIVATE -municode)
    endif()
endif()


