
set(instructions
    address.h
    bitset.h
    bool.h
    bytes.h
    caddr.h
    callable.h
    channel.h
    classifier.h
    double.h
    enum.h
    exception.h
    file.h
    flow.h
    hook.h
    integer.h
    interval.h
    iosrc.h
    list.h
    map.h
    network.h
    operators.h
    overlay.h
    port.h
    regexp.h
    set.h
    string.h
    struct.h
    time.h
    timer-mgr.h
    timer.h
    tuple.h
    vector.h
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

