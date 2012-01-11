
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(codegen
    debug-info-builder.cc
    codegen.cc
    type-builder.cc
    util.cc
    loader.cc
    storer.cc
    linker.cc
    coercer.cc
    stmt-builder.cc
)
