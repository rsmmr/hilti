# $Id$
#
# If ENABLE_LLVM is defined, we change the compiler setting to compile and
# link LLVM bitcode.

if ( ENABLE_LLVM )
   if ( LLVM_FOUND )
       message("building LLVM bitcode files")

       set(CMAKE_C_OUTPUT_EXTENSION ".bc")
       set(CMAKE_STATIC_LIBRARY_SUFFIX ".bca")
       set(CMAKE_C_COMPILER "clang")
       set(CMAKE_C_FLAGS "-std=gnu89 -g -fno-color-diagnostics -emit-llvm -D_GNU_SOURCE ${CMAKE_C_FLAGS} ${LLVM_COMPILE_FLAGS}")
       set(CMAKE_AR "llvm-ar")
       set(CMAKE_RANLIB "llvm-ranlib")

       MESSAGE(STATUS "LLVM C Compiler: " ${CMAKE_C_COMPILER})
       MESSAGE(STATUS "LLVM CFLAGS: " ${CMAKE_C_FLAGS})
#       MESSAGE(STATUS "LLVM CXX flags: " ${LLVM_CXX_COMPILE_FLAGS})
#       MESSAGE(STATUS "LLVM flags: " ${LLVM_C_COMPILE_FLAGS})
#       MESSAGE(STATUS "LLVM LD flags: " ${LLVM_LDFLAGS})
#       MESSAGE(STATUS "LLVM core libs: " ${LLVM_LIBS_CORE})
#       MESSAGE(STATUS "LLVM JIT libs: " ${LLVM_LIBS_JIT})
#       MESSAGE(STATUS "LLVM JIT objs: " ${LLVM_LIBS_JIT_OBJECTS})

   else ( LLVM_FOUND )
       message(FATAL_ERROR "LLVM build requested but LLVM installation not found.")

   endif ( LLVM_FOUND )

else ( ENABLE_LLVM )
       message("building native object files")
endif ( ENABLE_LLVM )
