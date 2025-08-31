cmake_minimum_required(VERSION 3.16)

project(c-hash VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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


