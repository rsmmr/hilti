
// Instructions controlling flow.

#include "instructions/define-instruction.h"

#include "passes/validator.h"

iBegin(flow, ReturnResult, "return.result")
    iTerminator()
    iOp1(optype::any, true)

    iValidate {
        auto hook = validator()->current<declaration::Hook>();

        if ( hook ) {
            error(nullptr, "return.result must not be used inside a hook; use hook.stop instead");
            return;
        }
    }

    iDoc("")
iEnd

iBegin(flow, ReturnVoid, "return.void")
    iTerminator()
    iValidate {
    }

    iDoc("")
iEnd

iBegin(flow, BlockEnd, "block.end")
    iTerminator()
    iValidate {
    }

    iDoc("Internal instruction marking the end of a block that doesn't have any other terminator.")
iEnd

iBegin(flow, CallVoid, "call")
    iOp1(optype::function, true)
    iOp2(optype::tuple, true)

    iValidate {
    }

    iDoc("")
iEnd

iBegin(flow, CallResult, "call")
    iTarget(optype::any)
    iOp1(optype::function, true)
    iOp2(optype::tuple, true)

    iValidate {
    }

    iDoc("")
iEnd

iBegin(flow, CallCallableResult, "call")
    iTarget(optype::any)
    iOp1(optype::refCallable, true)

    iValidate {
    }

    iDoc("")
iEnd

iBegin(flow, CallCallableVoid, "call")
    iOp1(optype::refCallable, true)

    iValidate {
    }

    iDoc("")
iEnd

iBegin(flow, Yield, "yield")
    iValidate {
    }

    iDoc(R"(
        Yields processing back to the current scheduler, to be resumed later.
        If running in a virtual thread other than zero, this instruction yields
        to other virtual threads running within the same physical thread. If
        running in virtual thread zero (or in non-threading mode), returns
        execution back to the calling C function (see interfacing with C).
    )")
iEnd

iBegin(flow, IfElse, "if.else")
    iTerminator()
    iOp1(optype::boolean, true)
    iOp2(optype::label, true)
    iOp3(optype::label, true)

    iValidate {
    }

    iDoc(R"(    
        Transfers control label *op2* if *op1* is true, and to *op3*
        otherwise.
    )")

iEnd

iBegin(flow, Jump, "jump")
    iTerminator()
    iOp1(optype::label, true)

    iValidate {
    }

    iDoc(R"(    
        Jumps unconditionally to label *op2*.
    )")

iEnd

iBeginH(flow, Switch, "switch")
    iTerminator()
    iOp1(optype::any, true)
    iOp2(optype::label, true)
    iOp3(optype::tuple, true)
iEndH
