# Project name and a few useful settings. Other commands can pick up the results
project(
  ecmlib
  VERSION 0.1.0
  DESCRIPTION "ECM blocks management library"
  LANGUAGES C CXX)

set(libsrc ecmlib.base.cpp ecmlib.encoder.cpp)



# this is the "object library" target: compiles the sources only once
add_library(objlib OBJECT ${libsrc})

# shared libraries need PIC
set_property(TARGET objlib PROPERTY POSITION_INDEPENDENT_CODE 1)
target_include_directories(objlib PRIVATE ../include)

# Make the shared library from the object
add_library(ecmlib_shared SHARED $<TARGET_OBJECTS:objlib>)
set_target_properties(ecmlib_shared PROPERTIES OUTPUT_NAME "ecm")
set_target_properties(ecmlib_shared PROPERTIES CXX_STANDARD 17)
set_target_properties(ecmlib_shared PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(ecmlib_shared PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

# Make the static library from the object
add_library(ecmlib_static STATIC $<TARGET_OBJECTS:objlib>)
set_target_properties(ecmlib_static PROPERTIES OUTPUT_NAME "ecm")
set_target_properties(ecmlib_static PROPERTIES CXX_STANDARD 17)
set_target_properties(ecmlib_static PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(ecmlib_static PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})