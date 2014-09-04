# $Id$

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

project(JustRX)

cmake_minimum_required(VERSION 2.6)
set(PROJECT_VERSION "0.1")

# set(CMAKE_VERBOSE_MAKEFILE "on")

# We create two versions of the "jrx-config" script: one for usage with
# installation and one without.
#
# TODO: Note that we can't "make install" yet.

    # Build-directory version.
set(CONFIG_LIBS "-ljrx")
set(CONFIG_LDFLAGS "-L${CMAKE_BINARY_DIR}/src ${CONFIG_LIBS}")
set(CONFIG_CFLAGS "-I${CMAKE_BINARY_DIR}/src")
set(CONFIG_CXXFLAGS "${CONFIG_CFLAGS}")
configure_file("bin/jrx-config.in" "bin/jrx-config-build")

    # Installation version.
set(CONFIG_LIBS "-ljrx")
set(CONFIG_LDFLAGS "-L${CMAKE_INSTALL_PREFIX}/lib ${CONFIG_LIBS}")
set(CONFIG_CFLAGS "-I${CMAKE_INSTALL_PREFIX}/include")
set(CONFIG_CXXFLAGS "${CONFIG_CFLAGS}")
configure_file("bin/jrx-config.in" "bin/jrx-config")

add_subdirectory(src)
add_subdirectory(doc)

if (${LLVM_ENABLED})
else ()
    # Only build the executable when we're not building the LLVM
    # version. Otherwise the compilers get mixed up.
    add_subdirectory(test)
endif ()


