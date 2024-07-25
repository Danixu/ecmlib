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

## Copy the binary files for testing
configure_file("cdda.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" COPYONLY)
configure_file("cdda_gap.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" COPYONLY)
configure_file("mode1.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" COPYONLY)
configure_file("mode1_gap.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" COPYONLY)
configure_file("mode1_raw.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" COPYONLY)
configure_file("mode2.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" COPYONLY)
configure_file("mode2_gap.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" COPYONLY)
configure_file("mode2_xa_gap.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" COPYONLY)
configure_file("mode2_xa1.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" COPYONLY)
configure_file("mode2_xa1_gap.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" COPYONLY)