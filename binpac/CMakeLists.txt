
set(BINPAC_VERSION "0.2")

set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(PATH_LIBBINPAC "${CMAKE_CURRENT_SOURCE_DIR}/libpac")
set(PATH_HILTI     "${CMAKE_CURRENT_SOURCE_DIR}/../hilti")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${autogen}/config.h)

add_custom_command(
   OUTPUT   ${autogen}/visitor-types.h
            ${autogen}/visitor-interface.h
   COMMAND  ${scripts}/make-visitor-interface binpac ${CMAKE_CURRENT_SOURCE_DIR}/nodes.decl ${autogen}
   DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/nodes.decl ${scripts}/make-visitor-interface
)

SET_SOURCE_FILES_PROPERTIES(${autogen}/visitor-interface.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/visitor-types.h     PROPERTIES GENERATED 1)

### Generate the Bison/Flex parser.

bison_target(Parser parser/parser.yy
             ${autogen}/parser.cc
             HEADER ${autogen}/parser.h
             VERBOSE ${autogen}/parser.output
             COMPILE_FLAGS "${BISON_FLAGS}")

flex_target(Scanner parser/scanner.ll ${autogen}/scanner.cc)

ADD_CUSTOM_TARGET(generate_parser
                  DEPENDS ${autogen}/parser.cc ${autogen}/scanner.cc)

SET_SOURCE_FILES_PROPERTIES(${autogen}/scanner.cc PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/parser.cc  PROPERTIES GENERATED 1)

### Auto-generate all the boiler-plate stuff for operators.

set(operators
    operators/integer.cc
)

add_custom_command(
   OUTPUT   ${autogen}/operators/operators-declare.h
            ${autogen}/operators/operators-visits.h
            ${autogen}/operators/operators-expression-builder.h
            ${autogen}/operators/operators-register.cc
   COMMAND  ${scripts}/make-operators -h ${CMAKE_CURRENT_SOURCE_DIR} ${autogen}/operators ${operators}
   DEPENDS  ${operators} operators/define-operator.h ${scripts}/make-operators
)

SET_SOURCE_FILES_PROPERTIES(${autogen}/operators-declare.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/operators-visits.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/operators-expression-builder.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/operators-register.cc  PROPERTIES GENERATED 1)

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen}/operators)

foreach(in ${operators})
   string(REPLACE ".cc" "" base "${in}")
   #string(REPLACE "" "${autogen}/operators" base "${base}")

   set(h  "${autogen}/${base}.h")
   set(cc "${autogen}/${base}.cc")

   set(operators_cc ${operators_cc} ${cc})

   add_custom_command(
       OUTPUT   ${h} ${cc}
       COMMAND  ${scripts}/expand-macros ${CMAKE_CURRENT_SOURCE_DIR}/operators/macros.dat ${CMAKE_CURRENT_SOURCE_DIR}/${in} ${autogen}/operators
       DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/${in} operators/define-operator.h ${scripts}/expand-macros ${CMAKE_CURRENT_SOURCE_DIR}/operators/macros.dat
   )

   SET_SOURCE_FILES_PROPERTIES(${cc} PROPERTIES GENERATED 1)

endforeach ()

### Build libpac.

add_library (binpac SHARED
    attribute.cc
    binpac.cc
    constant.cc
    constant-coercer.cc
    context.cc
    coercer.cc
    ctor.cc
    declaration.cc
    expression.cc
    function.cc
    id.cc
    module.cc
    operator.cc
    scope.cc
    statement.cc
    type.cc
    variable.cc
    visitor-interface.cc

    ${operators_cc}

    parser/driver.cc

    passes/printer.cc
    passes/validator.cc
    passes/id-resolver.cc
    passes/operator-resolver.cc
    passes/scope-builder.cc
    passes/scope-printer.cc

    ${autogen}/operators/operators-register.cc

    ${autogen}/scanner.cc
    ${autogen}/parser.cc

    ${autogen}/visitor-interface.h
    ${autogen}/visitor-types.h
    )

target_link_libraries(binpac ast)
