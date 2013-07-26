
#include "define-instruction.h"

#include "port.h"
#include "../module.h"

iBeginCC(port)
    iValidateCC(Equal) {
    }

    iDocCC(Equal, R"(
        Returns true if *op1* is equal to *op2*.
    )")
iEndCC

iBeginCC(port)
    iValidateCC(Protocol) {
        auto ty_target = as<type::Enum>(target->type());

        if ( util::strtolower(ty_target->id()->pathAsString()) != "hilti::protocol" )
            error(target, "target must be of type hilti::Protocol");
    }

    iDocCC(Protocol, R"(
        Returns the protocol of *op1*, which can currently be either
        ``Port::TCP`` or ``Port::UDP``.
    
    )")
iEndCC
