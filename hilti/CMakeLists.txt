
include(DefaultCompilerSettings)

file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" HILTI_VERSION LIMIT_COUNT 1)

set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")
set(ffi     "${CMAKE_CURRENT_BINARY_DIR}/autogen/ffi")

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${ffi})

include_directories(${LLVM_INCLUDE_DIR})

## Create config file.

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/hilti-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/autogen/hilti-config.h)

### Build libffi.

include_directories(${ffi})
add_subdirectory(codegen/libffi)

### Generate visitor interface and RTTI information.

add_custom_command(
   OUTPUT   ${autogen}/visitor-types.h
            ${autogen}/visitor-interface.h
            ${autogen}/visitor-rtti.cc
   COMMAND  ${scripts}/make-visitor-interface hilti ${CMAKE_CURRENT_SOURCE_DIR}/nodes.decl ${autogen}
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

add_custom_target(generate_parser_hilti
                  DEPENDS ${autogen}/parser.cc ${autogen}/scanner.cc)

set_source_files_properties(${autogen}/scanner.cc PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/parser.cc  PROPERTIES GENERATED 1)

set_property(SOURCE ${autogen}/scanner.cc APPEND PROPERTY COMPILE_FLAGS "-Wno-null-conversion")

### Generate the instruction declarations.

set(instructions
    instructions/address.h
    instructions/bitset.h
    instructions/bool.h
    instructions/bytes.h
    instructions/caddr.h
    instructions/callable.h
    instructions/channel.h
    instructions/classifier.h
    instructions/debug.h
    instructions/double.h
    instructions/enum.h
    instructions/exception.h
    instructions/file.h
    instructions/flow.h
    instructions/hook.h
    instructions/integer.h
    instructions/interval.h
    instructions/iosrc.h
    instructions/list.h
    instructions/map.h
    instructions/misc.h
    instructions/network.h
    instructions/operators.h
    instructions/overlay.h
    instructions/port.h
    instructions/profiler.h
    instructions/reference.h
    instructions/regexp.h
    instructions/set.h
    instructions/string.h
    instructions/struct.h
    instructions/time.h
    instructions/timer-mgr.h
    instructions/timer.h
    instructions/union.h
    instructions/thread.h
    instructions/tuple.h
    instructions/vector.h
)

add_custom_command(
   OUTPUT   ${autogen}/instructions.h
            ${autogen}/instructions-declare.h
            ${autogen}/instructions-define.cc
            ${autogen}/instructions-visits.h
            ${autogen}/instructions-stmt-builder.h
            ${autogen}/instructions-register.cc
   COMMAND  ${scripts}/make-instructions -h ${CMAKE_CURRENT_SOURCE_DIR} ${autogen} ${instructions}
   DEPENDS  instructions/define-instruction.h ${instructions} ${scripts}/make-instructions
)

foreach(hdr ${instructions})
   string(REPLACE ".h" ".cc" cc "${hdr}")
   string(REPLACE "instructions/" "${autogen}/instructions/" cc "${cc}")

   set(instructions_cc ${instructions_cc} ${cc})

   add_custom_command(
       OUTPUT   ${cc}
       COMMAND  ${scripts}/make-instructions -cc ${CMAKE_CURRENT_SOURCE_DIR} ${autogen} ${hdr}
       DEPENDS  instructions/define-instruction.h ${hdr} ${scripts}/make-instructions
   )

   set_source_files_properties(${cc} PROPERTIES GENERATED 1)
endforeach ()

set_source_files_properties(${autogen}/instructions.h PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/instructions-declare.h PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/instructions-visits.h PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/instructions-stmt-builder.h PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/instructions-register.cc  PROPERTIES GENERATED 1)
set_source_files_properties(${autogen}/instructions-define.cc  PROPERTIES GENERATED 1)

### Build libhilti.

add_library (hilti STATIC
    attribute.cc
    coercer.cc
    constant-coercer.cc
    constant.cc
    context.cc
    ctor.cc
    expression.cc
    flow-info.cc
    jit.cc
    hilti.cc
    instruction.cc
    module.cc
    statement.cc
    type.cc
    visitor-interface.cc
    declaration.cc
    function.cc
    options.cc

    builder/block.cc
    builder/module.cc

    instructions/bytes.cc
    instructions/callable.cc
    instructions/channel.cc
    instructions/classifier.cc
    instructions/flow.cc
    instructions/exception.cc
    instructions/hook.cc
    instructions/iosrc.cc
    instructions/misc.cc
    instructions/operators.cc
    instructions/optypes.cc
    instructions/port.cc
    instructions/profiler.cc
    instructions/struct.cc
    instructions/thread.cc
    instructions/tuple.cc
    instructions/union.cc

    parser/driver.cc

    passes/block-flattener.cc
    passes/block-normalizer.cc
    passes/collector.cc
    passes/id-resolver.cc
    passes/global-type-resolver.cc
    passes/instruction-resolver.cc
    passes/instruction-normalizer.cc
    passes/printer.cc
    passes/scope-builder.cc
    passes/validator.cc
    passes/cfg.cc
    passes/liveness.cc
    passes/id-replacer.cc
    passes/optimize-ctors.cc
    passes/optimize-peephole.cc

    codegen/abi.cc
    codegen/asm-annotater.cc
    codegen/codegen.cc
    codegen/coercer.cc
    codegen/debug-info-builder.cc
    codegen/field-builder.cc
    codegen/instructions/bool.cc
    codegen/instructions/bytes.cc
    codegen/instructions/misc.cc
    codegen/instructions/integer.cc
    codegen/instructions/operators.cc
    codegen/instructions/string.cc
    codegen/instructions/address.cc
    codegen/instructions/bitset.cc
    codegen/instructions/caddr.cc
    codegen/instructions/callable.cc
    codegen/instructions/channel.cc
    codegen/instructions/classifier.cc
    codegen/instructions/double.cc
    codegen/instructions/debug.cc
    codegen/instructions/enum.cc
    codegen/instructions/exception.cc
    codegen/instructions/file.cc
    codegen/instructions/flow.cc
    codegen/instructions/hook.cc
    codegen/instructions/interval.cc
    codegen/instructions/iosrc.cc
    codegen/instructions/list.cc
    codegen/instructions/map.cc
    codegen/instructions/network.cc
    codegen/instructions/overlay.cc
    codegen/instructions/port.cc
    codegen/instructions/profiler.cc
    codegen/instructions/reference.cc
    codegen/instructions/regexp.cc
    codegen/instructions/set.cc
    codegen/instructions/struct.cc
    codegen/instructions/time.cc
    codegen/instructions/timer-mgr.cc
    codegen/instructions/timer.cc
    codegen/instructions/thread.cc
    codegen/instructions/tuple.cc
    codegen/instructions/union.cc
    codegen/instructions/vector.cc
    codegen/linker.cc
    codegen/loader.cc
    codegen/optimizer.cc
    codegen/protogen.cc
    codegen/stmt-builder.cc
    codegen/storer.cc
    codegen/type-builder.cc
    codegen/unpacker.cc
    codegen/packer.cc
    codegen/util.cc

    ${autogen}/instructions.h
    ${autogen}/instructions-declare.h
    ${autogen}/instructions-visits.h
    ${autogen}/instructions-stmt-builder.h
    ${autogen}/instructions-register.cc
    ${autogen}/instructions-define.cc

    ${autogen}/visitor-rtti.cc

    ${autogen}/scanner.cc
    ${autogen}/parser.cc
    ${instructions_cc}

    $<TARGET_OBJECTS:ast>
    $<TARGET_OBJECTS:util>
    $<TARGET_OBJECTS:hilti-ffi>
)

add_dependencies(hilti generate_parser_hilti)
target_link_libraries(hilti ${LLVM_LIBS} ${LLVM_LDFLAGS})

include(ShowCompilerSettings)
