
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(scripts "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

include(EnableClang)

cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
project (root C CXX)

# Do this first so that we retain the original compiler settings.
add_subdirectory(libhilti)
add_subdirectory(libbinpac)

include(FindRequiredPackage)
FindRequiredPackage(FLEX)
FindRequiredPackage(BISON)
FindRequiredPackage(LLVM)

include (CheckIncludeFiles)
check_include_files (alloca.h HAVE_ALLOCA_H)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${LLVM_INCLUDE_DIR})

include(DefaultCompilerSettings)

add_subdirectory(hilti)
add_subdirectory(ast)
add_subdirectory(binpac)
add_subdirectory(tools)
add_subdirectory(util)

include(ShowCompilerSettings)
