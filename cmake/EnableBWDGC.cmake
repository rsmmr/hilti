# $Id$
#
# Changes the compiler settings to redirect all memory allocation to
# the BDW garbage collector.
#
# NOTE: If changing CFLAGS here, you might also want to change them
# for host applications by adapting what "hilti-config --cflags" outputs.

MESSAGE(STATUS "Compiling for BWD garbage collector")

# USE_GC signals to include <gc.h>.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_GC -I${CMAKE_SOURCE_DIR}/3rdparty/bdwgc/include")

# Redirect all memory management to the collector.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DREDIRECT_MALLOC=GC_malloc -DIGNORE_FREE")

# Docs say every file that can make use threads need to define these on Linux.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGC_THREADS -D_REENTRANT")

if (${LLVM_ENABLE_DEBUG})
    # For the debugging version, add further checks.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGC_DEBUG -DGC_ASSERTIONS")
endif ()

MESSAGE(STATUS "New CFLAGS with GC: " ${CMAKE_C_FLAGS})
