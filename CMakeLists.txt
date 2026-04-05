cmake_minimum_required(VERSION 3.28)
project(FFPlayCpp LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find Dependencies ---

# Find SDL2
# If using vcpkg, this works out of the box. Otherwise, you may need to set SDL2_DIR.
find_package(SDL2 CONFIG REQUIRED)

# Find FFmpeg
# On Windows, vcpkg provides a FindFFMPEG.cmake module. 
# We'll try find_package first, and fall back to pkg-config if not found.
find_package(FFMPEG QUIET)

if(NOT FFMPEG_FOUND)
    # Fallback to pkg-config (common on Linux/MSYS2)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec
        libavformat
        libavfilter
        libavdevice
        libavutil
        libswscale
        libswresample
    )
endif()

# --- Setup Target ---

# Collect source files and modules
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE MODULES "include/*.ixx")

# Create the executable
add_executable(ffplay_cpp ${SOURCES})

# Add C++20 modules
target_sources(ffplay_cpp
    PUBLIC
    FILE_SET CXX_MODULES
    FILES
        ${MODULES}
)

# Include directories
target_include_directories(ffplay_cpp PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Link dependencies
if(TARGET PkgConfig::FFMPEG)
    target_link_libraries(ffplay_cpp PRIVATE PkgConfig::FFMPEG)
else()
    target_link_libraries(ffplay_cpp PRIVATE ${FFMPEG_LIBRARIES})
    target_include_directories(ffplay_cpp PRIVATE ${FFMPEG_INCLUDE_DIRS})
endif()

target_link_libraries(ffplay_cpp PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
)
