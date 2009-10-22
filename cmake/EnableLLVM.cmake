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
       set(CMAKE_C_FLAGS "-emit-llvm ${CMAKE_C_FLAGS} ${LLVM_COMPILE_FLAGS}")
       set(CMAKE_AR "llvm-ar")
       set(CMAKE_RANLIB "llvm-ranlib")
       
   else ( LLVM_FOUND )
       message(FATAL_ERROR "LLVM build requested but LLVM installation not found.")
       
   endif ( LLVM_FOUND )
   
else ( ENABLE_LLVM )
       message("building native object files")
endif ( ENABLE_LLVM )
