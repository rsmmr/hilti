
include_directories(${CMAKE_SOURCE_DIR}/libhilti)

include(DefaultCompilerSettings)
include(EnableLLVMBitcode)

set(scripts "${CMAKE_SOURCE_DIR}/scripts")
set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")
set(hiltip  "${CMAKE_CURRENT_BINARY_DIR}/../tools/hiltip")
set(hiltic  "${CMAKE_CURRENT_BINARY_DIR}/../tools/hiltic")
set(spicyc  "${CMAKE_CURRENT_BINARY_DIR}/../tools/spicyc")

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

# Used below for mode-specific libraries.
set(c_debug_flags                "-DDEBUG -O0")
set(c_release_flags              "-O0") # Do not optimize. We optimize during linking, and doing it twice can lead to LLVM trouble.

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DJRX_USE_HILTI -I${CMAKE_SOURCE_DIR}/justrx/src")

### Generate the C prototypes.

add_custom_command(
    OUTPUT   ${autogen}/spicyhilti-hlt.h
    COMMAND  ${hiltip} -P -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/spicyhilti.hlt -o ${autogen}/spicyhilti-hlt.h
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/spicyhilti.hlt ${hiltip}
)

add_custom_command(
    OUTPUT   ${autogen}/spicy-hlt.h
    COMMAND  ${hiltip} -P -I ${CMAKE_CURRENT_SOURCE_DIR} ${autogen}/spicy.hlt -o ${autogen}/spicy-hlt.h
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/spicyhilti.hlt ${autogen}/spicy.hlt ${hiltip}
)

SET_SOURCE_FILES_PROPERTIES(${autogen}/spicy-hlt.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/spicyhilti-hlt.h PROPERTIES GENERATED 1)

### Generate HILTI code for Spicy standard library code.

add_custom_command(
    OUTPUT   ${autogen}/spicy.hlt
    COMMAND  ${spicyc} ${CMAKE_CURRENT_SOURCE_DIR}/spicy.spicy -o ${autogen}/spicy.hlt
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/spicy.spicy ${spicyc}
)

SET_SOURCE_FILES_PROPERTIES(${autogen}/spicy.hlt PROPERTIES GENERATED 1)

add_custom_target(spicy.hlt ALL DEPENDS ${autogen}/spicy.hlt)

### Generate bitcode for the library.

add_custom_command(
    OUTPUT   ${autogen}/spicyhilti.o
    COMMAND  ${hiltic} -b -C -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/spicyhilti.hlt -o ${autogen}/spicyhilti.o
    DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/spicyhilti.hlt ${hiltic}
)

# add_custom_command(
#     OUTPUT   ${autogen}/spicy.o
#     COMMAND  ${hiltic} -b -C -I ${CMAKE_CURRENT_SOURCE_DIR} ${autogen}/spicy.hlt -o ${autogen}/spicy.o
#     DEPENDS  ${autogen}/spicy.hlt ${hiltic}
# )

SET_SOURCE_FILES_PROPERTIES(${autogen}/spicyhilti.hlt PROPERTIES GENERATED 1)
# SET_SOURCE_FILES_PROPERTIES(${autogen}/spicy.hlt PROPERTIES GENERATED 1)

#### Build the library.

set(SRCS
    base64.c
    exceptions.c
    filter.c
    globals.c
    init.c
    libspicy.c
    mime.c
    misc.c
    render.c
    rtti.c
    sink.c

    3rdparty/libb64-1.2/src/cdecode.c
    3rdparty/libb64-1.2/src/cencode.c
    3rdparty/sha2/sha2.c
    3rdparty/rc4/rc4.c

    ${autogen}/spicy-hlt.h
    ${autogen}/spicyhilti-hlt.h
)

add_library(spicy-rt-dbg       STATIC ${SRCS} ${autogen}/spicyhilti.o)
add_library(spicy-rt           STATIC ${SRCS} ${autogen}/spicyhilti.o)

add_dependencies(spicy-rt-dbg  hiltic)
add_dependencies(spicy-rt      hiltic)

set_target_properties(spicy-rt-dbg PROPERTIES COMPILE_FLAGS ${c_debug_flags})
set_target_properties(spicy-rt     PROPERTIES COMPILE_FLAGS ${c_release_flags})

include(ShowCompilerSettings)

message(STATUS "Additional compiler flags for libspicy-rt release build: ${c_release_flags}")
message(STATUS "Additional compiler flags for libspicy-rt debug   build: ${c_debug_flags}")
