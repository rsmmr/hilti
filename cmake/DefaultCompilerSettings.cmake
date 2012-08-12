#
# Sets some standard compiler settings, such as using C++11 and
# libc++; and warnings-are-errors in debug mode.
#

MESSAGE(STATUS "Adapting compiler settings")

set(clang_debug_flags  "-DDEBUG -Wno-error=unused-function -Werror -O0")

set(clang_cflags       "-lcxxrt -L${LLVM_LIB_DIR} -fPIC -Qunused-arguments -fno-color-diagnostics -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
set(clang_cxxflags     "${clang_cflags} -stdlib=libc++ -std=c++0x")

set(CMAKE_C_FLAGS      "${CMAKE_C_FLAGS}   ${clang_cflags}")
set(CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS} ${clang_cxxflags}")
# set(CMAKE_LD_FLAGS     "${CMAKE_LD_FLAGS}")

if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    # Changing CMAKE_C_FLAGS_DEBUG does not have any effect here?
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${clang_debug_flags}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${clang_debug_flags}")
endif ()

if ( "${CMAKE_SYSTEM_NAME}" MATCHES "Darwin" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDARWIN")
endif ()
