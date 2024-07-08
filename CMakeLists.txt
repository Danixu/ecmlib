cmake_minimum_required(VERSION 3.27)

# list of source files
set(libsrc src/ecmlib.cpp)
set(libincludes include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) 
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Project name and a few useful settings. Other commands can pick up the results
project(
  ecmlib
  VERSION 0.1.0
  DESCRIPTION "ECM blocks management library"
  LANGUAGES C CXX)

# this is the "object library" target: compiles the sources only once
add_library(objlib OBJECT ${libsrc})

set_target_properties(objlib PROPERTIES CXX_STANDARD 17)
set_target_properties(objlib PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(objlib PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(objlib PROPERTIES PUBLIC_HEADER include/ecmlib.h)
# shared libraries need PIC
set_property(TARGET objlib PROPERTY POSITION_INDEPENDENT_CODE 1)
target_include_directories(objlib PRIVATE include)


# shared and static libraries built from the same object files
add_library(ecmlib_shared SHARED $<TARGET_OBJECTS:objlib>)
set_target_properties(ecmlib_shared PROPERTIES OUTPUT_NAME "ecm")
add_library(ecmlib_static STATIC $<TARGET_OBJECTS:objlib>)
set_target_properties(ecmlib_static PROPERTIES OUTPUT_NAME "ecm")

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