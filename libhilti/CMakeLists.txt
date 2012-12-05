
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(libhilti C)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/justrx/src)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=unused-function -Werror -fno-color-diagnostics")

IF( APPLE )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDARWIN")
ENDIF(APPLE)

# These are used below in set_target_properties.
set(c_debug_flags   "-DDEBUG -O0")
set(c_release_flags "-O2")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

include(FindRequiredPackage)
include(EnableLLVMBitcode)
include(EnableClangDebugging)

FindRequiredPackage(FLEX)
FindRequiredPackage(BISON)
find_package(PAPI)

if (${PAPI_FOUND} STREQUAL "TRUE" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_PAPI")
endif ()

set(scripts "${CMAKE_SOURCE_DIR}/scripts")
set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

set(hiltic "${CMAKE_CURRENT_BINARY_DIR}/../tools/hiltic-nojit")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_HILTI -I${CMAKE_SOURCE_DIR} -I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_SOURCE_DIR}/justrx/src")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_HILTI -std=gnu89 -D_GNU_SOURCE")

### Generate the C prototypes.

add_custom_command(
    OUTPUT   ${autogen}/hilti-hlt.h
    COMMAND  ${hiltic} -P -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/hilti.hlt -o ${autogen}/hilti-hlt.h
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/hilti.hlt ${hiltic}
)

SET_SOURCE_FILES_PROPERTIES(${autogen}/hilti-hlt.h PROPERTIES GENERATED 1)

### Generate type-info for the library.

add_custom_command(
    OUTPUT   ${autogen}/type-info.o
    COMMAND  ${hiltic} -b -C -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/type-info.hlt -o ${autogen}/type-info.o
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/type-info.hlt ${hiltic}
)

SET_SOURCE_FILES_PROPERTIES(${autogen}/type-info.bc PROPERTIES GENERATED 1)

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
    system.c classifier.c iosrc.c profiler.c channel.c main.c rtti.c

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

add_library(hilti-rt-dbg       STATIC ${SRCS} ${autogen}/type-info.o)
add_library(hilti-rt           STATIC ${SRCS} ${autogen}/type-info.o)

add_dependencies(hilti-rt-dbg      generate_jrx_parser build_asm hiltic-nojit)
add_dependencies(hilti-rt          generate_jrx_parser build_asm hiltic-nojit)

set_target_properties(hilti-rt-dbg PROPERTIES COMPILE_FLAGS ${c_debug_flags})
set_target_properties(hilti-rt     PROPERTIES COMPILE_FLAGS ${c_release_flags})

include(ShowCompilerSettings)

message(STATUS "Additional compiler flags for libhilti-rt release build: ${c_release_flags}")
message(STATUS "Additional compiler flags for libhilti-rt debug   build: ${c_debug_flags}")
