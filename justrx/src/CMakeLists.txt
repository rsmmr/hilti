# $Id$

find_package(BISON)
find_package(FLEX)
find_package(LLVM)

# We generate the parser files into the source directory so that we 
# can revision-control them.  
flex_target(rescan re-scan.l "${CMAKE_CURRENT_SOURCE_DIR}/re-scan.c") 
bison_target(reparse re-parse.y "${CMAKE_CURRENT_SOURCE_DIR}/re-parse.c")
add_flex_bison_dependency(rescan reparse)

set(CMAKE_C_FLAGS "-g -Wall -ansi -strict -std=c99")

include(EnableLLVM)

set(SRCS 
    ccl.c dfa.c dfa-interpreter-std.c dfa-interpreter-min.c jlocale.c 
    jrx.c nfa.c util.c
    
    # Generated.
    ${FLEX_rescan_OUTPUTS} ${BISON_reparse_OUTPUT_SOURCE}
    )

add_library(jrx STATIC ${SRCS})  
