
include_directories(${CMAKE_SOURCE_DIR}/libhilti)

include(DefaultCompilerSettings)
include(EnableLLVMBitcode)

set(scripts "${CMAKE_SOURCE_DIR}/scripts")
set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")
set(hiltip  "${CMAKE_CURRENT_BINARY_DIR}/../tools/hiltip")
set(hiltic  "${CMAKE_CURRENT_BINARY_DIR}/../tools/hiltic")
set(binpac  "${CMAKE_CURRENT_BINARY_DIR}/../tools/binpac++")

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

# Used below for mode-specific libraries.
set(c_debug_flags                "-DDEBUG -O0")
set(c_release_flags              "-O0") # Do not optimize. We optimize during linking, and doing it twice can lead to LLVM trouble.

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DJRX_USE_HILTI -I${CMAKE_SOURCE_DIR}/justrx/src")

### Generate the C prototypes.

add_custom_command(
    OUTPUT   ${autogen}/binpachilti-hlt.h
    COMMAND  ${hiltip} -P -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/binpachilti.hlt -o ${autogen}/binpachilti-hlt.h
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/binpachilti.hlt ${hiltip}
)

add_custom_command(
    OUTPUT   ${autogen}/binpac-hlt.h
    COMMAND  ${hiltip} -P -I ${CMAKE_CURRENT_SOURCE_DIR} ${autogen}/binpac.hlt -o ${autogen}/binpac-hlt.h
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/binpachilti.hlt ${autogen}/binpac.hlt ${hiltip}
)

SET_SOURCE_FILES_PROPERTIES(${autogen}/binpac-hlt.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/binpachilti-hlt.h PROPERTIES GENERATED 1)

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
    base64.c
    exceptions.c
    filter.c
    globals.c
    init.c
    libbinpac++.c
    mime.c
    misc.c
    render.c
    rtti.c
    sink.c

    3rdparty/libb64-1.2/src/cdecode.c
    3rdparty/libb64-1.2/src/cencode.c
    3rdparty/sha2/sha2.c
    3rdparty/rc4/rc4.c

    ${autogen}/binpac-hlt.h
    ${autogen}/binpachilti-hlt.h
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
