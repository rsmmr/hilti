
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_subdirectory(instructions)

add_library(codegen
    abi.cc
    debug-info-builder.cc
    codegen.cc
    type-builder.cc
    util.cc
    loader.cc
    storer.cc
    linker.cc
    coercer.cc
    unpacker.cc
    stmt-builder.cc
    asm-annotater.cc
)

target_link_libraries(codegen cg-instructions)
