# $Id$
#
# Changes the compiler settings to compile and link LLVM bitcode. If
# LLVM_ENABLE_DEBUG is defined, we compile a debug version (i.e., no
# optimization and "#define DEBUG"); otherwise a release version
# (i.e., enable optimization and "#define NDEBUG").

OPTION(LLVM_ENABLE_DEBUG "Compile a debug version when using LLVM/clang" NO)

if ( LLVM_FOUND )
    MESSAGE(STATUS "Building LLVM bitcode files")
    SET(LLVM_ENABLED 1)

    set(CMAKE_C_OUTPUT_EXTENSION ".bc")
    set(CMAKE_STATIC_LIBRARY_SUFFIX ".bca")
    set(CMAKE_C_COMPILER "clang")
        # Put warning options last so that we don't get overridden by predefined stuff.
    set(CMAKE_C_FLAGS "-std=gnu89 -g -fno-color-diagnostics -emit-llvm -D_GNU_SOURCE ${CMAKE_C_FLAGS} ${LLVM_COMPILE_FLAGS} -Wno-error=unused-function -Werror")
    set(CMAKE_AR "llvm-ar")
    set(CMAKE_RANLIB "llvm-ranlib")
    set(CMAKE_LD "llvm-ld")

    if (${LLVM_ENABLE_DEBUG})
        MESSAGE(STATUS "Compiling DEBUG version.")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -DDEBUG")
    else ()
        MESSAGE(STATUS "Compiling RELEASE version.")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -DNDEBUG")
    endif ()

    MESSAGE(STATUS "LLVM C Compiler: " ${CMAKE_C_COMPILER})
    MESSAGE(STATUS "LLVM CFLAGS: " ${CMAKE_C_FLAGS})
#   MESSAGE(STATUS "LLVM CXX flags: " ${LLVM_CXX_COMPILE_FLAGS})
#   MESSAGE(STATUS "LLVM flags: " ${LLVM_C_COMPILE_FLAGS})
#   MESSAGE(STATUS "LLVM LD flags: " ${LLVM_LDFLAGS})
#   MESSAGE(STATUS "LLVM core libs: " ${LLVM_LIBS_CORE})
#   MESSAGE(STATUS "LLVM JIT libs: " ${LLVM_LIBS_JIT})
#   MESSAGE(STATUS "LLVM JIT objs: " ${LLVM_LIBS_JIT_OBJECTS})

else ( LLVM_FOUND )
   MESSAGE(FATAL_ERROR "LLVM build requested but LLVM installation not found.")

endif ( LLVM_FOUND )
