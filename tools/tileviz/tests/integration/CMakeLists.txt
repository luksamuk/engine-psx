cmake_minimum_required(VERSION 3.20)

# Integration tests executable
add_executable(test_integration
    test_integration.c
    ${CMAKE_SOURCE_DIR}/src/log.c
    ${CMAKE_SOURCE_DIR}/src/tim_parser.c
    ${CMAKE_SOURCE_DIR}/src/level_loader.c
    ${CMAKE_SOURCE_DIR}/src/level_viewer.c
    ${CMAKE_SOURCE_DIR}/src/tile_renderer.c
    ${CMAKE_SOURCE_DIR}/src/util.c
)

target_link_libraries(test_integration PRIVATE raylib)

target_include_directories(test_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)
