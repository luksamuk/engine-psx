cmake_minimum_required(VERSION 3.20)
project(tile-visualization-tool VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build tests option
option(BUILD_TESTS "Build test suite" ON)

# Add executable
add_executable(tileviz
    src/main.c
    src/log.c
    src/tim_parser.c
    src/level_loader.c
    src/tile_renderer.c
    src/level_viewer.c
    src/util.c
)

# Raylib dependency
find_package(raylib REQUIRED)

target_link_libraries(tileviz PUBLIC raylib)

# Include directories
target_include_directories(tileviz PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# Compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tileviz PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# Output directory
set_target_properties(tileviz PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS tileviz
    RUNTIME DESTINATION bin
)

# Add tests directory if enabled
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()