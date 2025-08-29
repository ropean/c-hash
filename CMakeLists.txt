cmake_minimum_required(VERSION 3.16)

project(c-hash VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(c-hash
    src/main.cpp
)

if(WIN32)
    # Link bcrypt on Windows (name is lowercase for MinGW)
    target_link_libraries(c-hash PRIVATE bcrypt)
endif()

target_compile_definitions(c-hash PRIVATE NOMINMAX)

# For MinGW, enable wide-char entry point
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(c-hash PRIVATE -municode)
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
    target_sources(c-hash PRIVATE ${RC_OUT})
endif()


