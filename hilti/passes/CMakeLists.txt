
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library (passes
     collector.cc
     printer.cc
     id-resolver.cc
     instruction-resolver.cc
     scope-builder.cc
     validator.cc
     block-normalizer.cc
)
