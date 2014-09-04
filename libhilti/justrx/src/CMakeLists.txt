# $Id$

find_package(BISON)
find_package(FLEX)

set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

bison_target(JRXParser re-parse.y
             ${autogen}/re-parse.c
             VERBOSE ${autogen}/re-parse.output
             COMPILE_FLAGS "${BISON_FLAGS}")

flex_target(JRXScanner re-scan.l
             ${autogen}/re-scan.c
             COMPILE_FLAGS "--header-file=${autogen}/re-scan.h"
             )

ADD_CUSTOM_TARGET(generate_jrx_parser
                  DEPENDS ${autogen}/re-parse.c ${autogen}/re-scan.c)

SET_SOURCE_FILES_PROPERTIES(${autogen}/re-scan.c PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/re-parse.c  PROPERTIES GENERATED 1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}")

if ( ${LLVM_ENABLED} )
    include(EnableLLVMBitcode)
endif ()

set(SRCS
    ccl.c dfa.c dfa-interpreter-std.c dfa-interpreter-min.c jlocale.c
    jrx.c nfa.c util.c

    # Generated.
    ${autogen}/re-scan.c
    ${autogen}/re-parse.c
    )

set(all_deps generate_parser ${SRCS} ${OBJS} ${CMAKE_CURRENT_BINARY_DIR}/binpac.hlt.h)

if (${LLVM_ENABLE_DEBUG})
    add_library(jrxdbg STATIC ${SRCS})
    ADD_DEPENDENCIES(jrxdbg generate_parser)
else ()
    add_library(jrx STATIC ${SRCS})
    ADD_DEPENDENCIES(jrx generate_parser)
endif ()


