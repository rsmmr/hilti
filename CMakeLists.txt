
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(EnableClang)

cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
project(HILTI C CXX)

set(PROJECT_VERSION "0.2")

include(FindRequiredPackage)

set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")
set(pch     "${CMAKE_CURRENT_BINARY_DIR}/autogen/pch")
set(ffi     "${CMAKE_CURRENT_BINARY_DIR}/autogen/ffi")

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${pch})
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${ffi})

FindRequiredPackage(FLEX)
FindRequiredPackage(BISON)
FindRequiredPackage(LLVM)

include(DefaultCompilerSettings)
include(PCHSupport_26)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dnullptr=0 -I${LLVM_INCLUDE_DIR}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/hilti ${CMAKE_CURRENT_BINARY_DIR})

include(ShowCompilerSettings)

include ( CheckIncludeFiles )
check_include_files ( alloca.h HAVE_ALLOCA_H )

### Build libffi.

include_directories(${ffi})
add_subdirectory(hilti/codegen/libffi)

### Generate the Bison/Flex parser.

bison_target(Parser hilti/parser/parser.yy
             ${autogen}/parser.cc
             HEADER ${autogen}/parser.h
             VERBOSE ${autogen}/parser.output
             COMPILE_FLAGS "${BISON_FLAGS}")

flex_target(Scanner hilti/parser/scanner.ll ${autogen}/scanner.cc)

ADD_CUSTOM_TARGET(generate_parser
                  DEPENDS ${autogen}/parser.cc ${autogen}/scanner.cc)

SET_SOURCE_FILES_PROPERTIES(${autogen}/scanner.cc PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/parser.cc  PROPERTIES GENERATED 1)

### Generate the instruction declarations.

set(instructions
    hilti/instructions/address.h
    hilti/instructions/bitset.h
    hilti/instructions/bool.h
    hilti/instructions/bytes.h
    hilti/instructions/caddr.h
    hilti/instructions/callable.h
    hilti/instructions/channel.h
    hilti/instructions/classifier.h
    hilti/instructions/double.h
    hilti/instructions/enum.h
    hilti/instructions/exception.h
    hilti/instructions/file.h
    hilti/instructions/flow.h
    hilti/instructions/hook.h
    hilti/instructions/integer.h
    hilti/instructions/interval.h
    hilti/instructions/iosrc.h
    hilti/instructions/list.h
    hilti/instructions/map.h
    hilti/instructions/network.h
    hilti/instructions/operators.h
    hilti/instructions/port.h
    hilti/instructions/regexp.h
    hilti/instructions/set.h
    hilti/instructions/string.h
    hilti/instructions/struct.h
    hilti/instructions/time.h
    hilti/instructions/timer-mgr.h
    hilti/instructions/timer.h
    hilti/instructions/tuple.h
    hilti/instructions/vector.h
)

add_custom_command(
   OUTPUT   ${autogen}/instructions.h
            ${autogen}/instructions-declare.h
            ${autogen}/instructions-visits.h
            ${autogen}/instructions-stmt-builder.h
            ${autogen}/instructions-register.cc
   COMMAND  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/make-instructions -h ${CMAKE_CURRENT_SOURCE_DIR} ${autogen} ${instructions}
   DEPENDS  hilti/instructions/define-instruction.h ${instructions} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/make-instructions
)

foreach(hdr ${instructions})

   string(REPLACE ".h" ".cc" cc "${hdr}")
   string(REPLACE "hilti/instructions/" "${autogen}/instructions/" cc "${cc}")

   set(instructions_cc ${instructions_cc} ${cc})

   add_custom_command(
       OUTPUT   ${cc}
       COMMAND  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/make-instructions -cc ${CMAKE_CURRENT_SOURCE_DIR} ${autogen} ${hdr}
       DEPENDS  hilti/instructions/define-instruction.h ${hdr} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/make-instructions
   )

   SET_SOURCE_FILES_PROPERTIES(${cc} PROPERTIES GENERATED 1)

endforeach ()

SET_SOURCE_FILES_PROPERTIES(${autogen}/instructions.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/instructions-declare.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/instructions-visits.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/instructions-stmt-builder.h PROPERTIES GENERATED 1)
SET_SOURCE_FILES_PROPERTIES(${autogen}/instructions-register.cc  PROPERTIES GENERATED 1)

# For fast compilation, precompile the instructions header files.

foreach(hdr ${instructions})
    get_filename_component(basename ${hdr} NAME)
    get_filename_component(abshdr ${hdr} ABSOLUTE)

    ADD_PRECOMPILED_HEADER(${pch}/${basename}.pch ${abshdr} instructions_autogen)

    set(instructions_pch ${instructions_pch} ${pch}/${basename}.pch)
endforeach ()

### Build libhilti.

add_library (hilti
    ast/exception.cc
    ast/location.cc
    ast/logger.cc
    ast/node.cc
    ast/type.cc

    util/util.cc
    util/utf8proc.c

    hilti/coercer.cc
    hilti/constant-coercer.cc
    hilti/constant.cc
    hilti/ctor.cc
    hilti/expression.cc
    hilti/hilti.cc
    hilti/instruction.cc
    hilti/module.cc
    hilti/statement.cc
    hilti/type.cc
    hilti/visitor-interface.cc
    hilti/declaration.cc
    hilti/function.cc

    hilti/parser/driver.cc

    hilti/passes/block-normalizer.cc
    hilti/passes/collector.cc
    hilti/passes/id-resolver.cc
    hilti/passes/instruction-resolver.cc
    hilti/passes/printer.cc
    hilti/passes/scope-builder.cc
    hilti/passes/validator.cc

    hilti/codegen/abi.cc
    hilti/codegen/asm-annotater.cc
    hilti/codegen/codegen.cc
    hilti/codegen/coercer.cc
    hilti/codegen/debug-info-builder.cc
    hilti/codegen/instructions/bool.cc
    hilti/codegen/instructions/bytes.cc
    hilti/codegen/instructions/flow.cc
    hilti/codegen/instructions/integer.cc
    hilti/codegen/instructions/operators.cc
    hilti/codegen/instructions/string.cc
    hilti/codegen/instructions/address.cc
    hilti/codegen/instructions/bitset.cc
    hilti/codegen/instructions/caddr.cc
    hilti/codegen/instructions/callable.cc
    hilti/codegen/instructions/channel.cc
    hilti/codegen/instructions/classifier.cc
    hilti/codegen/instructions/double.cc
    hilti/codegen/instructions/enum.cc
    hilti/codegen/instructions/exception.cc
    hilti/codegen/instructions/file.cc
    hilti/codegen/instructions/hook.cc
    hilti/codegen/instructions/interval.cc
    hilti/codegen/instructions/iosrc.cc
    hilti/codegen/instructions/list.cc
    hilti/codegen/instructions/map.cc
    hilti/codegen/instructions/network.cc
    hilti/codegen/instructions/port.cc
    hilti/codegen/instructions/regexp.cc
    hilti/codegen/instructions/set.cc
    hilti/codegen/instructions/struct.cc
    hilti/codegen/instructions/time.cc
    hilti/codegen/instructions/timer-mgr.cc
    hilti/codegen/instructions/timer.cc
    hilti/codegen/instructions/tuple.cc
    hilti/codegen/instructions/vector.cc
    hilti/codegen/linker.cc
    hilti/codegen/loader.cc
    hilti/codegen/protogen.cc
    hilti/codegen/stmt-builder.cc
    hilti/codegen/storer.cc
    hilti/codegen/type-builder.cc
    hilti/codegen/unpacker.cc
    hilti/codegen/util.cc

    libhilti/utf8proc.c

    ${autogen}/instructions.h
    ${autogen}/instructions-declare.h
    ${autogen}/instructions-visits.h
    ${autogen}/instructions-stmt-builder.h
    ${autogen}/instructions-register.cc

    ${autogen}/scanner.cc
    ${autogen}/parser.cc
    ${instructions_cc}

    ### ${instructions_pch} Activate to generate PCH. But we need to remove
    ### the -l* options from the PCH command line.
)

ADD_DEPENDENCIES(hilti generate_parser)
ADD_DEPENDENCIES(hilti instructions_autogen)
ADD_DEPENDENCIES(hilti hilti-ffi)

## Tools

add_subdirectory(tools)
