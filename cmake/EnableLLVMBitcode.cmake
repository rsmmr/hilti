#
# Changes the C compiler settings to compile and link LLVM bitcode with clang.
#
# Note that LLVM has deprecated their "bitcode archives" (*.bca),
# which was their equivalent of static libraries managed with
# llvm-ar/llvm-ranlib. We now just combine multiple *.bc into one
# to build a "library". The wrapper below do by mimiking the ar/ranlib
# interface to cmake.

include(FindRequiredPackage)
include(EnableClang)

MESSAGE(STATUS "Building LLVM bitcode files")

set(CMAKE_C_OUTPUT_EXTENSION ".bc")
set(CMAKE_CXX_OUTPUT_EXTENSION ".bc")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".bc")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -emit-llvm")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -emit-llvm")
set(ENV{LLVM_CONFIG_EXEC} ${LLVM_CONFIG_EXEC})
set(CMAKE_AR "${scripts}/llvm-ar-wrapper")
set(CMAKE_RANLIB "${scripts}/llvm-ranlib-wrapper")
# set(CMAKE_LD "llvm-ld")
