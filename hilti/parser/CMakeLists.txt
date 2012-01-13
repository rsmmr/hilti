
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
)


bison_target(Parser parser.yy
             ${CMAKE_CURRENT_BINARY_DIR}/parser.cc
             HEADER ${CMAKE_CURRENT_BINARY_DIR}/parser.h
             VERBOSE ${CMAKE_CURRENT_BINARY_DIR}/parser.output
             COMPILE_FLAGS "${BISON_FLAGS}")

flex_target(Scanner scanner.ll ${CMAKE_CURRENT_BINARY_DIR}/scanner.cc)

ADD_CUSTOM_TARGET(generate_parser DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/parser.cc ${CMAKE_CURRENT_BINARY_DIR}/scanner.cc)
