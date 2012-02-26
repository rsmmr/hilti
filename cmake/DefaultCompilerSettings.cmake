#
# Sets some standard compiler settings, such as using C++11 and
# libc++; and warnings-are-errors in debug mode.
#

MESSAGE(STATUS "Adapting compiler settings")

set(clang_debug_flags  "-DDEBUG  -Wno-error=unused-function -Werror -O0")

set(clang_cflags       "-fno-color-diagnostics")
set(clang_cxxflags     "${cflags} -stdlib=libc++ -std=c++0x")

set(CMAKE_C_FLAGS      "${CMAKE_C_FLAGS}   ${clang_cflags}")
set(CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS} ${clang_cxxflags}")

if ( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    # Changing CMAKE_C_FLAGS_DEBUG does not have any effect here?
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${clang_debug_flags}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${clang_debug_flags}")
endif ()

if ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDARWIN")
endif ()
