cmake_minimum_required(VERSION 3.20)
project(tile-visualization-tool-tests VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find raylib for test dependencies
find_package(raylib REQUIRED)

# Build test executables
add_subdirectory(unit)
add_subdirectory(integration)
