# $Id$

find_package(BISON)
find_package(FLEX)

# We generate the parser files into the source directory so that we
# can revision-control them.
flex_target(rescan re-scan.l "${CMAKE_CURRENT_SOURCE_DIR}/re-scan.c")
bison_target(reparse re-parse.y "${CMAKE_CURRENT_SOURCE_DIR}/re-parse.c")
add_flex_bison_dependency(rescan reparse)

find_package(LLVM)
include_directories(${LLVM_INCLUDE_DIR})
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${LLVM_C_COMPILE_FLAGS})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")

if ( ${LLVM_ENABLED} )
    find_package(LLVM)
    include(EnableLLVM)
    include(EnableBWDGC)
endif ()

set(SRCS
    ccl.c dfa.c dfa-interpreter-std.c dfa-interpreter-min.c jlocale.c
    jrx.c nfa.c util.c

    dummy.cpp

    # Generated.
    ${FLEX_rescan_OUTPUTS} ${BISON_reparse_OUTPUT_SOURCE}
    )

set(all_deps ${SRCS} ${OBJS} ${CMAKE_CURRENT_BINARY_DIR}/binpac.hlt.h)

if (${LLVM_ENABLE_DEBUG})
    add_library(jrxdbg STATIC ${SRCS})
else ()
    add_library(jrx STATIC ${SRCS})
endif ()

link_directories(${LLVM_LIB_DIR})
target_link_libraries(jrx ${LLVM_LDFLAGS})
target_link_libraries(jrx ${LLVM_LIBS_CORE} ${LLVM_LIBS_JIT})
#set_target_properties(jrx PROPERTIES LINKER_LANGUAGE CXX)


