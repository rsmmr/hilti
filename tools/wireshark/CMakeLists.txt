# CMakeLists.txt
#
# $Id: CMakeLists.txt 31995 2010-02-24 22:32:10Z jmayer $
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

#project(binpacshark C CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../hilti ${CMAKE_CURRENT_BINARY_DIR}/../../hilti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libhilti ${CMAKE_CURRENT_BINARY_DIR}/../../libhilti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../binpac ${CMAKE_CURRENT_BINARY_DIR}/../../binpac)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libbinpac ${CMAKE_CURRENT_BINARY_DIR}/../../libbinpac)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../util)

# FIXME
set(WIRESHARK_INCLUDE_DIRS "/home/robin/work/wireshark-1.99.1")

#cmake_minimum_required(VERSION 2.6)
set(CMAKE_BACKWARDS_COMPATIBILITY 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
IF(EXISTS /usr/lib/wireshark/libwireshark1/plugins/)
set(CMAKE_INSTALL_LIBDIR /usr/lib/wireshark/libwireshark1/plugins/)
ELSE()
set(CMAKE_INSTALL_LIBDIR ~/.wireshark)
ENDIF()

#INCLUDE(UseMakeDissectorReg)
#include(CMakeForceCompiler)

set(GLIB2_MIN_VERSION 2.4.0)

IF(NOT DEFINED ENV{GLIB2_INCLUDE_DIRS})
find_package(GLIB2)
ELSE()
SET(GLIB2_INCLUDE_DIRS $ENV{GLIB2_INCLUDE_DIRS})
ENDIF()
include_directories (${GLIB2_INCLUDE_DIRS})

IF(NOT DEFINED WIRESHARK_INCLUDE_DIRS)
find_package(Wireshark)
ELSE()
SET(GLIB2_INCLUDE_DIRS $ENV{WIRESHARK_INCLUDE_DIRS})
ENDIF()
include_directories (${WIRESHARK_INCLUDE_DIRS})

IF(NOT DEFINED CRYPTO_INCLUDE_DIR)
find_package(Crypto)
ELSE()
SET(GLIB2_INCLUDE_DIRS $ENV{CRYPTO_INCLUDE_DIR})
ENDIF()
include_directories (${CRYPTO_INCLUDE_DIR})

set(LINK_MODE_LIB SHARED)
set(LINK_MODE_MODULE MODULE)


set(DISSECTOR_SRC
	packet-binpac.c
    binpacshark.cc
)

set(PLUGIN_FILES
	plugin.c
	${DISSECTOR_SRC}
)

set(CLEAN_FILES
	${PLUGIN_FILES}
)

if (WERROR)
	set_source_files_properties(
		${CLEAN_FILES}
		PROPERTIES
		COMPILE_FLAGS -Werror
	)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#register_dissector_files(plugin.c
#	plugin
#	${DISSECTOR_SRC}
#)

add_library(binpacshark ${LINK_MODE_MODULE}
	${PLUGIN_FILES}
)
set_target_properties(binpacshark PROPERTIES PREFIX "")
set_target_properties(binpacshark PROPERTIES LINK_FLAGS "${WS_LINK_FLAGS}")

IF(DEFINED MANUAL_LINK)
target_link_libraries(binpacshark)
ELSE()
target_link_libraries(binpacshark crypto)
ENDIF()

install(TARGETS binpacshark
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/plugins NAMELINK_SKIP
)

IF(DEFINED GNU_HOST)
 CMAKE_FORCE_C_COMPILER(  ${GNU_HOST}-gcc GNU)
 CMAKE_FORCE_CXX_COMPILER(${GNU_HOST}-g++ GNU)
 set(CMAKE_STRIP "${GNU_HOST}-strip")
ENDIF()

IF(DEFINED STATIC_GCC_BUILD)
 set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
ENDIF()

add_custom_command(TARGET binpacshark
 POST_BUILD
 COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded binpacshark.so
)

# Add HILTI/BinPAC libraries.

find_package(PAPI)

if ( PAPI_FOUND )
    set(optional_libs ${PAPI_LIBRARIES})
endif ()

# FIXME
set(optional_libs "papi")

set(plugin_hilti_libs    hilti hilti-jit ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt-native.a)
set(plugin_binpac_libs   binpacxx)

if ( "${CMAKE_BUILD_TYPE}" MATCHES "Debug" )
    set(plugin_hilti_libs  ${plugin_hilti_libs} ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt-dbg.bc)
    set(plugin_binpac_libs ${plugin_binpac_libs} ${PROJECT_BINARY_DIR}/libbinpac/libbinpac-rt-dbg.bc)
else ()
    set(plugin_hilti_libs  ${plugin_hilti_libs} ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt.bc)
    set(plugin_binpac_libs ${plugin_binpac_libs} ${PROJECT_BINARY_DIR}/libbinpac/libbinpac-rt.bc)
endif ()

set(plugin_deps hilti-config hilti binpacxx libhilti-rt-dbg.bc libbinpac-rt-dbg.bc libhilti-rt.bc libbinpac-rt.bc)

target_link_libraries(binpacshark ${plugin_hilti_libs})
target_link_libraries(binpacshark ${plugin_binpac_libs})
target_link_libraries(binpacshark ${optional_libs})
add_dependencies(binpacshark ${autogen}/bro.pac2.h ${autogen}/libbro.hlt.h ${autogen}/libbro.hlt.o ${plugin_deps})


