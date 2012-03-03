
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/parser
                    ${CMAKE_CURRENT_BINARY_DIR}/parser
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
)

set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

add_subdirectory(parser)
add_subdirectory(passes)
add_subdirectory(codegen)
add_subdirectory(instructions)

add_library (hilti
     ${CMAKE_CURRENT_BINARY_DIR}/parser/parser.cc
     ${CMAKE_CURRENT_BINARY_DIR}/parser/scanner.cc
     parser/driver.cc
     module.cc
     constant.cc
     ctor.cc
     constant-coercer.cc
     coercer.cc
     instruction.cc
     statement.cc
     type.cc
     visitor-interface.cc
     hilti.cc
     expression.cc
)

target_link_libraries(hilti passes codegen instructions)

SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/parser/scanner.cc PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/parser/parser.cc  PROPERTIES GENERATED 1)

ADD_DEPENDENCIES(hilti generate_parser)
ADD_DEPENDENCIES(hilti instructions_autogen)

