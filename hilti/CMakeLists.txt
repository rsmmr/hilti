
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/parser
                    ${CMAKE_CURRENT_BINARY_DIR}/parser
)

add_subdirectory(parser)
add_subdirectory(passes)
add_subdirectory(codegen)

link_directories    (${HILTI_BINARY_DIR}/hilti)

add_library (hilti
     ${CMAKE_CURRENT_BINARY_DIR}/parser/parser.cc
     ${CMAKE_CURRENT_BINARY_DIR}/parser/scanner.cc
     instructions/all.cc
     parser/driver.cc
     module.cc
     constant.cc
     constant-coercer.cc
     coercer.cc
     instruction.cc
     statement.cc
     type.cc
     visitor-interface.cc
)

SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/parser/scanner.cc PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/parser/parser.cc  PROPERTIES GENERATED 1)

ADD_DEPENDENCIES(hilti generate_parser)
