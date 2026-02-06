cmake_minimum_required(VERSION 3.20)
project(tile_viewer_tests VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

add_executable(test_viewer test_viewer.c
                       test_tim_types.c
                       test_tim_parser.c
                       test_level_loader.c)

target_link_libraries(test_viewer PUBLIC raylib)
target_include_directories(test_viewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

enable_testing()
add_test(NAME viewer_test COMMAND test_viewer)
