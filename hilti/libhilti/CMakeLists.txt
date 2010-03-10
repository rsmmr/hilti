# $Id$

cmake_minimum_required(VERSION 2.6)

# set(CMAKE_VERBOSE_MAKEFILE "on")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH}) 
find_package(LLVM)
find_package(Pcap)

project(libhilti)
set(PROJECT_VERSION "0.1")

add_subdirectory(justrx) # must come before EnableLLVM.

set(CMAKE_C_FLAGS "-g -std=c99 -I${CMAKE_SOURCE_DIR} -I${CMAKE_SOURCE_DIR}/justrx/src -I{PCAP_CFLAGS}")

include(EnableLLVM)

set(SRCS
    exceptions.c utf8proc.c init.c memory.c string.c
    int.c double.c bool.c tuple.c channel.c enum.c
    bytes.c config.c schedulers.c thread_context.c run.c
    addr.c port.c vector.c list.c net.c regexp.c debug.c
    bitset.c context.c caddr.c struct.c timer.c map_set.c
    pktsrc-pcap.c
    
    module/fmt.c module/print.c module/abort.c
    
    3rdparty/libpqueue/src/pqueue.c
    )

ADD_CUSTOM_TARGET(header ALL DEPENDS module/_hilti.h)

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
   OUTPUT  ${CMAKE_CURRENT_SOURCE_DIR}/module/_hilti.h
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/module/hilti.hlt
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/libhilti-prototypes
   ARGS    ${CMAKE_CURRENT_SOURCE_DIR}/../tools ${CMAKE_CURRENT_SOURCE_DIR}/module/hilti.hlt ${CMAKE_CURRENT_SOURCE_DIR}/module/_hilti.h
   
   ${CMAKE_CURRENT_SOURCE_DIR}/libhilti.hlt
   ) 

if ( ENABLE_LLVM )
    set(OBJS
        ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.bc.o
        ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
        )

else ( ENABLE_LLVM )

    # Need to convert the BC files into native object files.
    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.o
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.bc.o
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/bc-to-native
        ARGS    ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.bc.o ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.o
        ) 

    # Generate type information for types defines in type-info.hlt
    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/type-info.o
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/bc-to-native
        ARGS    ${CMAKE_CURRENT_BINARY_DIR}/type-info.bc.o ${CMAKE_CURRENT_BINARY_DIR}/type-info.o
        ) 

    set(OBJS
        ${CMAKE_CURRENT_BINARY_DIR}/call_hilti.o
        ${CMAKE_CURRENT_BINARY_DIR}/type-info.o
        )


endif ( ENABLE_LLVM )


add_library(hilti STATIC ${SRCS} ${OBJS})  
add_library(hiltimain STATIC ${SRCS} ${OBJS} main.c)  

set(DOXYGEN ${CMAKE_CURRENT_SOURCE_DIR}/Doxygen)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxygen/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
add_custom_target(doc-libhilti doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
