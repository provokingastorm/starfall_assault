cmake_minimum_required(VERSION 3.8...4.4)
project(starfall_assault VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Locate raylib -----------------------------------------------------
# 1) If raylib was installed via vcpkg (recommended), this just works when
#    you configure with -DCMAKE_TOOLCHAIN_FILE=<vcpkg root>/scripts/buildsystems/vcpkg.cmake
# 2) Otherwise, fall back to fetching and building raylib from source.
find_package(raylib QUIET)

if (NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(starfall_assault main.c)
target_link_libraries(starfall_assault PRIVATE raylib)

if (WIN32)
    target_link_libraries(starfall_assault PRIVATE winmm gdi32 user32 shell32)
endif()

# Use the Windows subsystem without a console window in Release builds
# (comment this out if you want a console for printf/debug output)
if (MSVC)
    target_link_options(starfall_assault PRIVATE $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>)
endif()
