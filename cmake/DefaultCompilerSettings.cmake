#
# Sets standard compiler settings we use across all submodules.
#

include(EnableClang)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${LLVM_INCLUDE_DIR})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(cflags_common "")
set(cflags_common  "${cflags_common} -I${CMAKE_SOURCE_DIR} -I${CMAKE_CURRENT_BINARY_DIR}")
set(cflags_common  "${cflags_common} -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
set(cflags_common  "${cflags_common} -Wall -pedantic -Wno-potentially-evaluated-expression -Wno-unused-function")
set(cflags_common  "${cflags_common} -Wno-c99-extensions -Wno-flexible-array-extensions -Wno-gnu-variable-sized-type-not-at-end -Wno-format-pedantic")

set(cflags_Darwin  "-DDARWIN")
    # On Mac force the triple, as OS and custom clang sometimes disagree.
set(cflags_Darwin  "${cflags_Darwin} -Xclang -triple -Xclang ${LLVM_TRIPLE}")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${cflags_common} ${cflags_${CMAKE_SYSTEM_NAME}} -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${cflags_common} ${cflags_${CMAKE_SYSTEM_NAME}} -std=c++11 -fno-rtti")

include(FindRequiredPackage)

## Check for PAPI.

find_package(PAPI)

if (${PAPI_FOUND} STREQUAL "TRUE" )
    set(HAVE_PAPI "1")
else ()
    set(HAVE_PAPI "0")
endif ()

### Check for perftools.

find_package(GooglePerftools)

if ("${GOOGLEPERFTOOLS_FOUND}" STREQUAL "TRUE" )
    set(HAVE_PERFTOOLS "1")
else ()
    set(HAVE_PERFTOOLS "0")
endif ()

# MESSAGE(STATUS "Adapting compiler settings")
#
# set(clang_debug_flags    "-DDEBUG -D_DEBUG -Wall -Wno-error=unused-function -Werror -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls")
# set(clang_release_flags  "-DNDEBUG -O3")
#
# set(clang_cflags_Linux               "")
# set(clang_ldflags_Linux              "")
# set(clang_shared_linker_flags_Linux  "")
#
#  # On Mac force the triple, as OS and custom clang sometimes disagree.
# set(clang_cflags_Darwin              "-DDARWIN -Xclang -triple -Xclang ${LLVM_TRIPLE}")
# set(clang_ldflags_Darwin             "")
# set(clang_shared_linker_flags_Darwin "-undefined dynamic_lookup") # Don't warn about undefined symbols.
#
# set(clang_cflags              "${clang_cflags_${CMAKE_SYSTEM_NAME}} -L${LLVM_LIB_DIR} -fPIC -Qunused-arguments -Wno-error=logical-op-parentheses -fno-color-diagnostics -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
# set(clang_cxxflags            "${clang_cflags} -stdlib=libc++ -std=c++0x -frtti")
# set(clang_ldflags             "${clang_ldflags_${CMAKE_SYSTEM_NAME}} -lc++")
# set(clang_shared_linker_flags "${clang_shared_linker_flags_${CMAKE_SYSTEM_NAME}}")
#
# set(CMAKE_C_FLAGS               "${CMAKE_C_FLAGS}   ${clang_cflags}")
# set(CMAKE_CXX_FLAGS             "${CMAKE_CXX_FLAGS} ${clang_cxxflags}")
# set(CMAKE_LD_FLAGS              "${CMAKE_LD_FLAGS}  ${clang_ldflags}")
# set(CMAKE_SHARED_LINKER_FLAGS   "${CMAKE_CMAKE_SHARED_LINKER_FLAGS} ${clang_shared_linker_flags}")
#
# if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
#     # Changing CMAKE_C_FLAGS_DEBUG does not have any effect here?
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${clang_debug_flags}")
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${clang_debug_flags}")
# else ()
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${clang_release_flags}")
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${clang_release_flags}")
# endif ()
#
# #IF( APPLE )
# #   # Without this, the Darwin linker tries to resolve all symbols at link time (even though from other shared libraries).
# #   SET(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -flat_namespace -undefined suppress")
# #ENDIF(APPLE)
#
