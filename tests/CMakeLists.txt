cmake_minimum_required(VERSION 3.28)
project(tests)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

## Static Build
add_executable(tests_static tests.cpp)
set_target_properties(tests_static PROPERTIES CXX_STANDARD 17)
target_link_libraries(tests_static PRIVATE ecmlib_static)
target_include_directories(tests_static PUBLIC "../src" "../libs/spdlog/include")

## Static Build
add_executable(tests_shared tests.cpp)
set_target_properties(tests_shared PROPERTIES CXX_STANDARD 17)
target_link_libraries(tests_shared PRIVATE ecmlib_shared)
target_include_directories(tests_shared PUBLIC "../src" "../libs/spdlog/include")