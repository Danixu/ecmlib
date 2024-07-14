cmake_minimum_required(VERSION 3.28)
project(tests)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE ecmlib_static)
target_include_directories(tests PUBLIC "../src" "../libs/spdlog/include")
set_target_properties(tests PROPERTIES CXX_STANDARD 17)
set_target_properties(tests PROPERTIES OUTPUT_NAME "test")