#
# Changes the C compiler settings to use clang.
#
# Also sets some further standard compiler settings, such as using C++11
# and libc++; and warnings-are-errors in debug mode.
#

include(FindRequiredPackage)
FindRequiredPackage(LLVM)

MESSAGE(STATUS "Using clang as C/C++ compiler")

set(CMAKE_C_COMPILER   "${LLVM_CLANG_EXEC}")
set(CMAKE_CXX_COMPILER "${LLVM_CLANGXX_EXEC}")

