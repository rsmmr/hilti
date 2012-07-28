
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
project (root)

include(FindRequiredPackage)
FindRequiredPackage(FLEX)
FindRequiredPackage(BISON)

include(EnableClang)
include(DefaultCompilerSettings)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include(ShowCompilerSettings)

set(scripts "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

add_subdirectory(tools)
add_subdirectory(util)
add_subdirectory(ast)
add_subdirectory(binpac)
