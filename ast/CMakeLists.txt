
include_directories (${HILTI_SOURCE_DIR})

add_library (ast location.cc node.cc type.cc logger.cc exception.cc)
