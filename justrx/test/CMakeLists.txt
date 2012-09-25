# $Id$

set(CMAKE_C_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../src")

add_executable(retest retest.c)
target_link_libraries(retest jrx)

add_executable(testregex testregex.c)
target_link_libraries(testregex jrx)

add_executable(fibtest fibtest.c)
target_link_libraries(fibtest jrx)

add_executable(llvmtest llvmtest.c)
target_link_libraries(llvmtest jrx)

add_executable(jit_test jit_test.c)
target_link_libraries(jit_test jrx)
