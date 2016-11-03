
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project (root C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(scripts "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

# Marker that we're part of the HILTI build tree.
set(BUILDING_HILTI "1")

# Do this first so that we retain the original compiler settings.

# include(FindRequiredPackage)
# FindRequiredPackage(FLEX)
# FindRequiredPackage(BISON)

# include (CheckIncludeFiles)
# check_include_files (alloca.h HAVE_ALLOCA_H)

# include(DefaultCompilerSettings)

# include(HiltiConfig)

message(STATUS "=== Setting up libhilti/")
add_subdirectory(libhilti)

message(STATUS "=== Setting up hilti/")
add_subdirectory(hilti)

message(STATUS "=== Setting up ast/")
add_subdirectory(ast)

message(STATUS "=== Setting up tools/")
add_subdirectory(tools)

message(STATUS "=== Setting up utils/")
add_subdirectory(util)

message(STATUS "=== Setting up libspicy/")
add_subdirectory(libspicy)

message(STATUS "=== Setting up spicy/")
add_subdirectory(spicy)

message(STATUS "=== Setting up bro/")
add_subdirectory(bro)
