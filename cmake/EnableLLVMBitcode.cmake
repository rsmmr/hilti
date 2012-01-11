#
# Changes the C compiler settings to compile and link LLVM bitcode with clang.
#

include(FindRequiredPackage)
include(EnableClang)

MESSAGE(STATUS "Building LLVM bitcode files")

set(CMAKE_C_OUTPUT_EXTENSION ".bc")
set(CMAKE_CXX_OUTPUT_EXTENSION ".bc")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".bca")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -emit-llvm")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -emit-llvm")
set(CMAKE_AR "llvm-ar")
set(CMAKE_RANLIB "llvm-ranlib")
set(CMAKE_LD "llvm-ld")
