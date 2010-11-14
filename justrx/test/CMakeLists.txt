# $Id$

if ( ENABLE_LLVM ) # FIXME: compilation with llvm-gcc doesn't work right now

else ( ENABLE_LLVM )

    set(CMAKE_C_FLAGS "-g -Wall -I${CMAKE_CURRENT_SOURCE_DIR}/../src")
    set(CMAKE_C_LDFLAGS "-L${CMAKE_CURRENT_BINARY_DIR}/../src -ljrx")

    add_executable(retest retest.c)
    target_link_libraries(retest jrx)

    add_executable(testregex testregex.c)
    target_link_libraries(testregex jrx)

endif ( ENABLE_LLVM )

