
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(scripts "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

# Marker that we're part of the HILTI build tree.
set(BUILDING_HILTI "1")

include(EnableClang)

cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
project (root C CXX)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

# Do this first so that we retain the original compiler settings.
add_subdirectory(libhilti)
add_subdirectory(libbinpac)

include(FindRequiredPackage)
FindRequiredPackage(FLEX)
FindRequiredPackage(BISON)
FindRequiredPackage(LLVM)

include (CheckIncludeFiles)
check_include_files (alloca.h HAVE_ALLOCA_H)

include_directories(${LLVM_INCLUDE_DIR})
include_directories(${FLEX_INCLUDE_DIR})

include(DefaultCompilerSettings)

set( CMAKE_VERBOSE_MAKEFILE on )

include(HiltiConfig)

add_subdirectory(hilti)
add_subdirectory(ast)
add_subdirectory(binpac)
add_subdirectory(tools)
add_subdirectory(util)
add_subdirectory(bro)

include(ShowCompilerSettings)
