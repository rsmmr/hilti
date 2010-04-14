# $Id$

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH}) 
find_package(LLVM)
find_package(HILTI)

project(libbinpac)
set(PROJECT_VERSION "0.1")

set(CMAKE_C_FLAGS "-g ${HILTI_CFLAGS}")

include(EnableLLVM)

set(OBJS ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o)

set(SRCS binpac-intern.c)

# Generate type information for libbinpac.
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/type-info.hlt
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen-type-info
    ARGS    ${CMAKE_CURRENT_SOURCE_DIR}/type-info.hlt ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
    ) 

add_library(binpac STATIC ${SRCS} ${OBJS})  

