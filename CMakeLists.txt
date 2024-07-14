cmake_minimum_required(VERSION 3.28)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

project(main)
add_subdirectory(src)
add_subdirectory(tests)

#if(NOT CMAKE_BUILD_TYPE)
#  set(CMAKE_BUILD_TYPE Release)
#endif()

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
#set(CMAKE_CXX_FLAGS_DEBUG "-g")
#set(CMAKE_CXX_FLAGS_RELEASE "-O2 -s")

# Only do these if this is the main project, and not if it is included through add_subdirectory
#if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
#  set(CMAKE_CXX_EXTENSIONS OFF)
#  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

  # The executable code is here
#  add_subdirectory(src)
#endif()