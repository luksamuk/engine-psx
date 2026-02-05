cmake_minimum_required(VERSION 3.20)

# Unit tests executable
add_executable(test_tim_types
    tests/unit/test_tim_types.c
    src/log.c
    src/tim_parser.c
    src/level_loader.c
    src/util.c
)

target_link_libraries(test_tim_types PRIVATE raylib)

target_include_directories(test_tim_types PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(test_tim_types PRIVATE
    ${CMAKE_C_FLAGS}
)

# Enable test discovery
enable_testing()

# Add custom test target
add_custom_target(unit_tests
    COMMAND ${test_tim_types}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS test_tim_types
)

# Register test
add_test(NAME TimTypesTests COMMAND test_tim_types)