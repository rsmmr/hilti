
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(EnableClang)

cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
project (root C CXX)

include(FindRequiredPackage)
FindRequiredPackage(FLEX)
FindRequiredPackage(BISON)

include (CheckIncludeFiles)
check_include_files (alloca.h HAVE_ALLOCA_H)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include(DefaultCompilerSettings)

set(scripts "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

add_subdirectory(hilti)
add_subdirectory(ast)
add_subdirectory(binpac)
add_subdirectory(tools)
add_subdirectory(util)

include(ShowCompilerSettings)
