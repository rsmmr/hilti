
set(instructions
    flow.h
    integer.h
    string.h
    bool.h
    bytes.h
)

add_custom_command(
   OUTPUT   ${autogen}/instructions.h
            ${autogen}/instructions-declare.h
            ${autogen}/instructions-visits.h
            ${autogen}/instructions-stmt-builder.h
            ${autogen}/instructions-register.cc
   COMMAND  ${CMAKE_CURRENT_SOURCE_DIR}/make-instructions ${CMAKE_CURRENT_SOURCE_DIR} ${autogen} ${instructions}
   DEPENDS  define-instruction.h ${instructions} ${CMAKE_CURRENT_SOURCE_DIR}/make-instructions
)

add_custom_target(instructions_autogen
    DEPENDS ${autogen}/instructions.h
            ${autogen}/instructions-declare.h
            ${autogen}/instructions-visits.h
            ${autogen}/instructions-stmt-builder.h
            ${autogen}/instructions-register.cc
)

add_library (instructions ${autogen}/instructions-register.cc)

