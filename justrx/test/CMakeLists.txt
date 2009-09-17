# $Id$


set(CMAKE_C_FLAGS "-g -Wall -ansi -strict -std=c99 -I${CMAKE_CURRENT_SOURCE_DIR}/../src")
set(CMAKE_C_LDFLAGS "-L${CMAKE_CURRENT_BINARY_DIR}/../src -ljrx")

add_executable(retest retest.c)
add_executable(testregex testregex.c)
