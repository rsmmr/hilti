#
# Note that this is a standalone CMakeLists.txt that's *not* included
# by the top-level one. That's because we change the compiler settings
# here and don't want that to propagate across other components.
#

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(libhilti C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

include(FindRequiredPackage)
include(EnableLLVMBitcode)
include(DefaultCompilerSettings)

FindRequiredPackage(FLEX)
FindRequiredPackage(BISON)
find_package(PAPI)

if (${PAPI_FOUND} STREQUAL "TRUE" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_PAPI")
endif ()

set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

set(hiltic "${CMAKE_CURRENT_BINARY_DIR}/../tools/hiltic")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_HILTI -I${CMAKE_SOURCE_DIR} -I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_SOURCE_DIR}/justrx/src")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_HILTI -std=gnu89 -D_GNU_SOURCE")

### Generate the C prototypes.

add_custom_command(
    OUTPUT   ${autogen}/hilti-hlt.h
    COMMAND  ${hiltic} -P -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/hilti.hlt -o ${autogen}/hilti-hlt.h
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/hilti.hlt ${hiltic}
)

SET_SOURCE_FILES_PROPERTIES(${autogen}/hilti-hlt.h PROPERTIES GENERATED 1)

### Generate the justrx parser.

bison_target(JRXParser justrx/src/re-parse.y
             ${autogen}/re-parse.c
             HEADER ${autogen}/re-parse.h
             VERBOSE ${autogen}/re-parse.output
             COMPILE_FLAGS "${BISON_FLAGS}")

flex_target(JRXScanner justrx/src/re-scan.l
             ${autogen}/re-scan.c
             COMPILE_FLAGS "--header-file=${autogen}/re-scan.h"
             )

ADD_CUSTOM_TARGET(generate_jrx_parser
                  DEPENDS ${autogen}/re-parse.c ${autogen}/re-scan.c)

SET_SOURCE_FILES_PROPERTIES(${autogen}/re-scan.c PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/re-parse.c  PROPERTIES GENERATED 1)

### Generate the library.

ENABLE_LANGUAGE(ASM)

set(SRCS callable.c config.c context.c debug.c globals.c init.c memory_.c int.c
    tuple.c string.c utf8proc.c bytes.c exceptions.c util.c print.c
    bool.c addr.c bitset.c caddr.c double.c enum.c interval.c
    net.c port.c time.c hook.c timer.c threading.c list.c fiber.c
    vector.c map_set.c struct.c regexp.c tqueue.c file.c cmdqueue.c
    system.c classifier.c iosrc.c profiler.c channel.c

    module/fmt.c
    module/misc.c

    justrx/src/ccl.c
    justrx/src/dfa-interpreter-min.c
    justrx/src/dfa-interpreter-std.c
    justrx/src/dfa.c
    justrx/src/jlocale.c
    justrx/src/jrx.c
    justrx/src/nfa.c
    justrx/src/util.c

    3rdparty/libpqueue/src/pqueue.c
    3rdparty/libtask/context.c

    ${autogen}/hilti-hlt.h
    ${autogen}/re-parse.c
    ${autogen}/re-scan.c
)

# Need to compile this ASM file separately as we can't turn it into
# bitcode.
add_custom_command(
    OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/asm.o
    COMMAND  ${LLVM_CLANG_EXEC} -c ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libtask/asm.S -o ${CMAKE_CURRENT_BINARY_DIR}/asm.o
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libtask/asm.S
)

add_custom_command(
    OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/libhilti-rt-native.a
    COMMAND  ar cr ${CMAKE_CURRENT_BINARY_DIR}/libhilti-rt-native.a ${CMAKE_CURRENT_BINARY_DIR}/asm.o
    DEPENDS  ${CMAKE_CURRENT_BINARY_DIR}/asm.o
)

add_custom_target(build_asm
                  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libhilti-rt-native.a)

if ( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    add_library(hilti-rt-dbg           STATIC ${SRCS})
    add_library(hilti-rt-main-dbg      STATIC main.c ${SRCS})
    add_dependencies(hilti-rt-dbg      generate_jrx_parser build_asm)
    add_dependencies(hilti-rt-main-dbg generate_jrx_parser build_asm)
else ()
    add_library(hilti-rt           STATIC ${SRCS})
    add_library(hilti-rt-main      STATIC main.c ${SRCS})
    add_dependencies(hilti-rt      generate_jrx_parser build_asm)
    add_dependencies(hilti-rt-main generate_jrx_parser build_asm)
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

