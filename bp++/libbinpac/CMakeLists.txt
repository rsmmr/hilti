# $Id$

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(LLVM)
find_package(HILTI)

project(libbinpac)
set(PROJECT_VERSION "0.1")

set(CMAKE_C_FLAGS "-g ${HILTI_CFLAGS} -I ${CMAKE_CURRENT_BINARY_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libb64-1.2/include")

include(EnableLLVM)

set(OBJS ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o)

set(SRCS binpac-intern.c binpac.c sink.c mime.c filter.c filter_base64.c exceptions.c

         3rdparty/libb64-1.2/src/cdecode.c)

# Generate type information for libbinpac.
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/type-info.hlt
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen-type-info
    ARGS    ${CMAKE_CURRENT_SOURCE_DIR}/type-info.hlt ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
    )

# Generate the C header for what's defined in hilti.hlt.
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/binpac.hlt.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/binpac.hlt
    COMMAND hiltic
    ARGS    -P ${CMAKE_CURRENT_SOURCE_DIR}/binpac.hlt >${CMAKE_CURRENT_BINARY_DIR}/binpac.hlt.h
    )

add_library(binpac STATIC ${SRCS} ${OBJS} ${CMAKE_CURRENT_BINARY_DIR}/binpac.hlt.h)
