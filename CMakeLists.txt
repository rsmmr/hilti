
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(EnableClang)

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(HILTI C CXX)

include(FindRequiredPackage)

# Check cache value first to avoid displaying "Found sed" messages everytime
#if (NOT SED_EXE)
#    find_program(SED_EXE sed)
#    if (NOT SED_EXE)
#        message(FATAL_ERROR "Could not find required dependency: sed")
#    else ()
#        message(STATUS "Found sed: ${SED_EXE}")
#    endif ()
#endif ()

FindRequiredPackage(FLEX)
FindRequiredPackage(BISON)
FindRequiredPackage(LLVM)

include(DefaultCompilerSettings)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dnullptr=0 -I${LLVM_INCLUDE_DIR}")

set(PROJECT_VERSION "0.2")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(ast)
add_subdirectory(util)
add_subdirectory(hilti)
add_subdirectory(tools)

include(ShowCompilerSettings)
