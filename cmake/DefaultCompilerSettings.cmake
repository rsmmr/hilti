#
# Sets some standard compiler settings, such as using C++11 and
# libc++; and warnings-are-errors in debug mode.
#

MESSAGE(STATUS "Adapting compiler settings")

set(clang_debug_flags  "-DDEBUG -Wno-error=unused-function -Werror -O0")

set(clang_cflags_Linux               "-lcxxrt -ldl") # FIXME: Why does not work in ldflags?
set(clang_ldflags_Linux              "")
set(clang_shared_linker_flags_Linux  "")

set(clang_cflags_Darwin              "-DDARWIN")
set(clang_ldflags_Darwin             "")
set(clang_shared_linker_flags_Darwin "-undefined dynamic_lookup") # Don't warn about undefined symbols.

set(clang_cflags              "${clang_cflags_${CMAKE_SYSTEM_NAME}} -L${LLVM_LIB_DIR} -fPIC -Qunused-arguments -Wno-error=logical-op-parentheses -fno-color-diagnostics -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
set(clang_cxxflags            "${clang_cflags} -stdlib=libc++ -std=c++0x -frtti")
set(clang_ldflags             "${clang_ldflags_${CMAKE_SYSTEM_NAME}} -lc++")
set(clang_shared_linker_flags "${clang_shared_linker_flags_${CMAKE_SYSTEM_NAME}}")

set(CMAKE_C_FLAGS               "${CMAKE_C_FLAGS}   ${clang_cflags}")
set(CMAKE_CXX_FLAGS             "${CMAKE_CXX_FLAGS} ${clang_cxxflags}")
set(CMAKE_LD_FLAGS              "${CMAKE_LD_FLAGS}  ${clang_ldflags}")
set(CMAKE_SHARED_LINKER_FLAGS   "${CMAKE_CMAKE_SHARED_LINKER_FLAGS} ${clang_shared_linker_flags}")

if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    # Changing CMAKE_C_FLAGS_DEBUG does not have any effect here?
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${clang_debug_flags}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${clang_debug_flags}")
endif ()

#IF( APPLE )
#   # Without this, the Darwin linker tries to resolve all symbols at link time (even though from other shared libraries).
#   SET(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -flat_namespace -undefined suppress")
#ENDIF(APPLE)

