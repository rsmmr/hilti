# $Id$

cmake_minimum_required(VERSION 2.6)

# set(CMAKE_VERBOSE_MAKEFILE "on")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(LLVM)
find_package(Pcap)

project(libhilti)
set(PROJECT_VERSION "0.1")

set(LLVM_ENABLED 1)

add_subdirectory(justrx) # Must come before EnableLLVM.

include(EnableLLVM)
include(EnableBWDGC)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR} -I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_SOURCE_DIR}/justrx/src ${PCAP_CFLAGS}")

set(SRCS
    exceptions.c utf8proc.c init.c memory.c string.c int.c double.c
    bool.c tuple.c channel.c enum.c bytes.c config.c threading.c
    addr.c port.c vector.c list.c net.c regexp.c debug.c bitset.c
    context.c caddr.c struct.c timer.c map_set.c iosrc-pcap.c
    continuation.c globals.c hook.c file.c cmdqueue.c time.c
    interval.c profiler.c classifier.c

    module/fmt.c module/print.c module/misc.c

    3rdparty/libpqueue/src/pqueue.c
    )

# FIXME: We should allow for multiple LLs and build a loop around
# the stuff below.
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.bc.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/call_hilti.ll
    COMMAND llvm-as
    ARGS    -f ${CMAKE_CURRENT_SOURCE_DIR}/call_hilti.ll -o ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.bc.o
    )

# Generate type information for libhilti.
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/type-info.hlt
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen-type-info
    ARGS    ${CMAKE_CURRENT_SOURCE_DIR}/../tools ${CMAKE_CURRENT_SOURCE_DIR}/type-info.hlt ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
    )

# Generate C prototypes for definitions in hilti.hlt.
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/hilti.hlt.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/module/hilti.hlt
    COMMAND hiltic
    ARGS    -P ${CMAKE_CURRENT_SOURCE_DIR}/module/hilti.hlt >${CMAKE_CURRENT_BINARY_DIR}/hilti.hlt.h
    )

ADD_CUSTOM_TARGET(header ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hilti.hlt.h)

set(OBJS
    ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.bc.o
    ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
)

# Get all the files which go into libgcmt-lib.a.
FILE(GLOB_RECURSE LIBGC "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/bdwgc/CMakeFiles/gcmt-lib.dir/*")

set(all_deps  ${SRCS} ${OBJS} ${LIBGC} ${CMAKE_CURRENT_BINARY_DIR}/hilti.hlt.h)
set(main_deps main.c)

if (${LLVM_ENABLE_DEBUG})
    add_library(hiltidbg     STATIC ${all_deps})
    add_library(hiltimaindbg STATIC ${main_deps})
else ()
    add_library(hilti     STATIC ${all_deps})
    add_library(hiltimain STATIC ${main_deps})
endif ()

