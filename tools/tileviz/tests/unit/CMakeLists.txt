cmake_minimum_required(VERSION 3.20)
project(tile_viewer_tests VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

# Common source files for all tests
set(TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/src/log.c
    ${CMAKE_SOURCE_DIR}/src/tim_parser.c
    ${CMAKE_SOURCE_DIR}/src/level_loader.c
    ${CMAKE_SOURCE_DIR}/src/level_viewer.c
    ${CMAKE_SOURCE_DIR}/src/tile_renderer.c
    ${CMAKE_SOURCE_DIR}/src/util.c
)

# Test: TIM types
add_executable(test_tim_types test_tim_types.c ${TEST_SOURCES})
target_include_directories(test_tim_types PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_tim_types PUBLIC raylib)

# Test: TIM parser
add_executable(test_tim_parser test_tim_parser.c ${TEST_SOURCES})
target_include_directories(test_tim_parser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_tim_parser PUBLIC raylib)

# Test: Level loader
add_executable(test_level_loader test_level_loader.c ${TEST_SOURCES})
target_include_directories(test_level_loader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_level_loader PUBLIC raylib)

# Test: Viewer (needs raylib)
add_executable(test_viewer test_viewer.c ${TEST_SOURCES})
target_include_directories(test_viewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_viewer PUBLIC raylib)

enable_testing()
add_test(NAME tim_types_test COMMAND test_tim_types)
add_test(NAME tim_parser_test COMMAND test_tim_parser)
add_test(NAME level_loader_test COMMAND test_level_loader)
add_test(NAME viewer_test COMMAND test_viewer)