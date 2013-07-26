
set(BINPAC_VERSION "0.2")

set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

# include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../hilti ${CMAKE_CURRENT_BINARY_DIR}/../hilti)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/binpac-config.h.in ${autogen}/binpac-config.h)

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

ADD_CUSTOM_TARGET(generate_parser_binpac
                  DEPENDS ${autogen}/parser.cc ${autogen}/scanner.cc)

SET_SOURCE_FILES_PROPERTIES(${autogen}/scanner.cc PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/parser.cc  PROPERTIES GENERATED 1)

### Auto-generate all the boiler-plate stuff for operators.

set(operators
    operators/address.cc
    operators/bool.cc
    operators/bytes.cc
    operators/double.cc
    operators/enum.cc
    operators/function.cc
    operators/integer.cc
    operators/interval.cc
    operators/iterator.cc
    operators/list.cc
    operators/sink.cc
    operators/time.cc
    operators/tuple.cc
    operators/unit.cc
    operators/vector.cc
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

add_library (binpacxx STATIC
    attribute.cc
    binpac++.cc
    coercer.cc
    constant-coercer.cc
    constant.cc
    context.cc
    context-ctor.cc
    ctor.cc
    declaration.cc
    expression.cc
    function.cc
    grammar.cc
    id.cc
    module.cc
    operator.cc
    production.cc
    scope.cc
    statement.cc
    type.cc
    variable.cc
    visitor-interface.cc
    options.cc

    ${operators_cc}

    codegen/code-builder.cc
    codegen/codegen.cc
    codegen/parser-builder.cc
    codegen/type-builder.cc

    codegen/operators/address.cc
    codegen/operators/bitset.cc
    codegen/operators/bool.cc
    codegen/operators/bytes.cc
    codegen/operators/double.cc
    codegen/operators/double.cc
    codegen/operators/enum.cc
    codegen/operators/function.cc
    codegen/operators/integer.cc
    codegen/operators/interval.cc
    codegen/operators/iterator.cc
    codegen/operators/list.cc
    codegen/operators/map.cc
    codegen/operators/network.cc
    codegen/operators/port.cc
    codegen/operators/regexp.cc
    codegen/operators/set.cc
    codegen/operators/sink.cc
    codegen/operators/string.cc
    codegen/operators/time.cc
    codegen/operators/tuple.cc
    codegen/operators/unit.cc
    codegen/operators/vector.cc

    parser/driver.cc

    passes/grammar-builder.cc
    passes/id-resolver.cc
    passes/overload-resolver.cc
    passes/normalizer.cc
    passes/operator-resolver.cc
    passes/printer.cc
    passes/scope-builder.cc
    passes/scope-printer.cc
    passes/unit-scope-builder.cc
    passes/validator.cc

    ${autogen}/operators/operators-register.cc

    ${autogen}/scanner.cc
    ${autogen}/parser.cc

    ${autogen}/visitor-interface.h
    ${autogen}/visitor-types.h
    ${autogen}/binpac-config.h

    $<TARGET_OBJECTS:ast>
    $<TARGET_OBJECTS:util>
    )

ADD_DEPENDENCIES(binpacxx hilti generate_parser_binpac)
target_link_libraries(binpacxx hilti)
