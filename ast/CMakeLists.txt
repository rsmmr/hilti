
add_library (ast SHARED
    exception.cc
    location.cc
    logger.cc
    node.cc
    type.cc
)

target_link_libraries(ast util)

