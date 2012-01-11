#
# Note that this is a standalone CMakeLists.txt that's *not* included
# by the top-level one. That's because we change the compiler settings
# here and don't want that to propagate across other components.
#

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(libhilti C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

include(EnableLLVMBitcode)
include(DefaultCompilerSettings)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR} -I${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu89 -D_GNU_SOURCE")

set(SRCS
    config.c context.c debug.c globals.c init.c memory.c
    int.c tuple.c string.c utf8proc.c bytes.c exceptions.c
    util.c print.c
    )

if ( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    add_library(hilti-rt-dbg      STATIC ${SRCS})
    add_library(hilti-rt-main-dbg STATIC main.c ${SRCS})
else ()
    add_library(hilti-rt          STATIC ${SRCS})
    add_library(hilti-rt-main     STATIC main.c ${SRCS})
endif ()

include(ShowCompilerSettings)

########### Old stuff, may reuse later.

# add_subdirectory(justrx) # Must come before EnableLLVM.


# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR} -I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_SOURCE_DIR}/justrx/src ${PCAP_CFLAGS}")

# if (${PAPI_FOUND} STREQUAL "TRUE" )
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_PAPI")
# endif ()


# FIXME: We should allow for multiple LLs and build a loop around
# the stuff below.
#add_custom_command(
#    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.bc.o
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/call_hilti.ll
#    COMMAND llvm-as
#    ARGS    -f ${CMAKE_CURRENT_SOURCE_DIR}/call_hilti.ll -o ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.bc.o
#    )
#
## Generate type information for libhilti.
#add_custom_command(
#    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/type-info.hlt
#    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen-type-info
#    ARGS    ${CMAKE_CURRENT_SOURCE_DIR}/../tools ${CMAKE_CURRENT_SOURCE_DIR}/type-info.hlt ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
#    )
#
## Generate C prototypes for definitions in hilti.hlt.
#add_custom_command(
#    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/hilti.hlt.h
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/module/hilti.hlt
#    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run-hiltic
#    ARGS    -P ${CMAKE_CURRENT_SOURCE_DIR}/module/hilti.hlt >${CMAKE_CURRENT_BINARY_DIR}/hilti.hlt.h
#    )
#
## Generate a file with the PAPI compiler flags. This is a hack for
## hilti-config as long as we don't have a "normal" install yet that
## could do a *.in replacement.
#add_custom_command(
#    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/papi-libs.cfg
#    COMMAND echo
#    ARGS    ${PAPI_LIBRARIES} >${CMAKE_CURRENT_BINARY_DIR}/papi-libs.cfg
#    )
#
#ADD_CUSTOM_TARGET(header ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hilti.hlt.h ${CMAKE_CURRENT_BINARY_DIR}/papi-libs.cfg)
#
#set(OBJS
#    ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.bc.o
#    ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
#)

# set(all_deps ${SRCS} ${OBJS} ${PAPI_LIBRARIES} ${CMAKE_CURRENT_BINARY_DIR}/hilti.hlt.h)

