
include(DefaultCompilerSettings)

file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" SPICY_VERSION LIMIT_COUNT 1)

set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

## Create config file.

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/spicy-config.h.in ${autogen}/spicy-config.h)

### Generate visitor interface and RTTI information.

add_custom_command(
   OUTPUT   ${autogen}/visitor-types.h
            ${autogen}/visitor-interface.h
            ${autogen}/visitor-rtti.cc
   COMMAND  ${scripts}/make-visitor-interface spicy ${CMAKE_CURRENT_SOURCE_DIR}/nodes.decl ${autogen}
   DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/nodes.decl ${scripts}/make-visitor-interface
)

set_source_files_properties(${autogen}/visitor-interface.h PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/visitor-types.h     PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/visitor-rtti.cc     PROPERTIES GENERATED 1)

### Generate the Bison/Flex parser.

FindRequiredPackage(FLEX)
FindRequiredPackage(BISON)

include_directories(${FLEX_INCLUDE_DIR})

bison_target(Parser parser/parser.yy
             ${autogen}/parser.cc
             HEADER ${autogen}/parser.h
             VERBOSE ${autogen}/parser.output
             COMPILE_FLAGS "${BISON_FLAGS}")

flex_target(Scanner parser/scanner.ll ${autogen}/scanner.cc)

add_custom_target(generate_parser_spicy
                  DEPENDS ${autogen}/parser.cc ${autogen}/scanner.cc)

set_source_files_properties(${autogen}/scanner.cc PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/parser.cc  PROPERTIES GENERATED 1)

set_property(SOURCE ${autogen}/scanner.cc APPEND PROPERTY COMPILE_FLAGS "-Wno-null-conversion")

### Generate the operator implementations.

set(operators
    operators/address.cc
    operators/bool.cc
    operators/bitfield.cc
    operators/bytes.cc
    operators/double.cc
    operators/enum.cc
    operators/function.cc
    operators/integer.cc
    operators/interval.cc
    operators/iterator.cc
    operators/list.cc
    operators/map.cc
    operators/optional.cc
    operators/set.cc
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
   DEPENDS  ${scripts}/make-operators
)

set_source_files_properties(${autogen}/operators-declare.h PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/operators-visits.h PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/operators-expression-builder.h PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/operators-register.cc  PROPERTIES GENERATED 1)

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen}/operators)

foreach(in ${operators})
   string(REPLACE ".cc" "" base "${in}")

   set(h  "${autogen}/${base}.h")
   set(cc "${autogen}/${base}.cc")

   set(operators_cc ${operators_cc} ${cc})

   add_custom_command(
       OUTPUT   ${h} ${cc}
       COMMAND  ${scripts}/expand-macros ${CMAKE_CURRENT_SOURCE_DIR}/operators/macros.dat ${CMAKE_CURRENT_SOURCE_DIR}/${in} ${autogen}/operators
       DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/${in} ${scripts}/expand-macros ${CMAKE_CURRENT_SOURCE_DIR}/operators/macros.dat
   )

   set_source_files_properties(${h}  PROPERTIES GENERATED 1)
   set_source_files_properties(${cc} PROPERTIES GENERATED 1)

endforeach ()

### Build libspicy.

add_library (spicy STATIC
    attribute.cc
    spicy.cc
    coercer.cc
    constant-coercer.cc
    constant.cc
    context-ctor.cc
    context.cc
    ctor.cc
    declaration.cc
    expression.cc
    function.cc
    grammar.cc
    id.cc
    jit.cc
    module.cc
    operator.cc
    options.cc
    production.cc
    scope.cc
    statement.cc
    type.cc
    variable.cc
    visitor-interface.cc

    ${operators_cc}

    codegen/code-builder.cc
    codegen/codegen.cc
    codegen/parser-builder.cc
    codegen/type-builder.cc
    codegen/synchronizer.cc
    codegen/composer.cc

    codegen/operators/address.cc
    codegen/operators/bitset.cc
    codegen/operators/bool.cc
    codegen/operators/bitfield.cc
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
    codegen/operators/optional.cc
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

    ${autogen}/spicy-config.h
    ${autogen}/operators/operators-register.cc
    ${autogen}/parser.cc
    ${autogen}/scanner.cc
    ${autogen}/visitor-interface.h
    ${autogen}/visitor-rtti.cc
    ${autogen}/visitor-types.h

    $<TARGET_OBJECTS:ast>
    $<TARGET_OBJECTS:util>
    )

add_dependencies(spicy hilti generate_parser_spicy)
target_link_libraries(spicy hilti)
