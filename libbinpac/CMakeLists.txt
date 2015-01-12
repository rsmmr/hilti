
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
project(libbinpac C)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libhilti ${CMAKE_CURRENT_BINARY_DIR}/../libhilti)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=unused-function -Werror -fno-color-diagnostics")

IF( APPLE )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDARWIN")
    # On Mac force the triple, as OS and custom clang sometimes disagree.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Xclang -triple -Xclang ${LLVM_TRIPLE}")
ENDIF(APPLE)

# These are used below in set_target_properties.
set(c_debug_flags   "-DDEBUG -O0")
set(c_release_flags "-O3")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

include(FindRequiredPackage)
include(EnableLLVMBitcode)
include(EnableClangDebugging)

set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

set(hiltic "${CMAKE_CURRENT_BINARY_DIR}/../tools/hiltic")
set(binpac "${CMAKE_CURRENT_BINARY_DIR}/../tools/binpac++")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_HILTI -I${CMAKE_SOURCE_DIR} -I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_SOURCE_DIR}/justrx/src")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_HILTI -std=gnu89 -D_GNU_SOURCE")

### Generate the C prototypes.

add_custom_command(
    OUTPUT   ${autogen}/binpac-hlt.h
    COMMAND  ${hiltic} -P -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/binpachilti.hlt -o ${autogen}/binpac-hlt.h
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/binpachilti.hlt ${hiltic}
)

SET_SOURCE_FILES_PROPERTIES(${autogen}/binpac-hlt.h PROPERTIES GENERATED 1)

### Generate HILTI code for BinPAC standard library code.

add_custom_command(
    OUTPUT   ${autogen}/binpac.hlt
    COMMAND  ${binpac} ${CMAKE_CURRENT_SOURCE_DIR}/binpac.pac2 -o ${autogen}/binpac.hlt
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/binpac.pac2 ${binpac}
)

SET_SOURCE_FILES_PROPERTIES(${autogen}/binpac.hlt PROPERTIES GENERATED 1)

add_custom_target(binpac.hlt ALL DEPENDS ${autogen}/binpac.hlt)

### Generate bitcode for the library.

add_custom_command(
    OUTPUT   ${autogen}/binpachilti.o
    COMMAND  ${hiltic} -b -C -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/binpachilti.hlt -o ${autogen}/binpachilti.o
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/binpachilti.hlt ${hiltic}
)

# add_custom_command(
#     OUTPUT   ${autogen}/binpac.o
#     COMMAND  ${hiltic} -b -C -I ${CMAKE_CURRENT_SOURCE_DIR} ${autogen}/binpac.hlt -o ${autogen}/binpac.o
#     DEPENDS  ${autogen}/binpac.hlt ${hiltic}
# )

SET_SOURCE_FILES_PROPERTIES(${autogen}/binpachilti.hlt PROPERTIES GENERATED 1)
# SET_SOURCE_FILES_PROPERTIES(${autogen}/binpac.hlt PROPERTIES GENERATED 1)

#### Build the library.

set(SRCS
    exceptions.c
    filter.c
    libbinpac++.c
    mime.c
    sink.c
    base64.c
    globals.c
    misc.c
    rtti.c
    render.c

    3rdparty/libb64-1.2/src/cdecode.c
    3rdparty/libb64-1.2/src/cencode.c

    ${autogen}/binpac-hlt.h
)

add_library(binpac-rt-dbg       STATIC ${SRCS} ${autogen}/binpachilti.o)
add_library(binpac-rt           STATIC ${SRCS} ${autogen}/binpachilti.o)

add_dependencies(binpac-rt-dbg  hiltic)
add_dependencies(binpac-rt      hiltic)

set_target_properties(binpac-rt-dbg PROPERTIES COMPILE_FLAGS ${c_debug_flags})
set_target_properties(binpac-rt     PROPERTIES COMPILE_FLAGS ${c_release_flags})

include(ShowCompilerSettings)

message(STATUS "Additional compiler flags for libbinpac-rt release build: ${c_release_flags}")
message(STATUS "Additional compiler flags for libbinpac-rt debug   build: ${c_debug_flags}")
