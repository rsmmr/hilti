
// Instructions controlling flow.

#include "instructions/define-instruction.h"

#include "passes/validator.h"

iBeginH(flow, ReturnResult, "return.result")
    iTerminator()
    iOp1(optype::any, true)
iEndH

iBeginH(flow, ReturnVoid, "return.void")
    iTerminator()
iEndH

iBeginH(flow, BlockEnd, "block.end")
    iTerminator()
iEndH

iBeginH(flow, CallVoid, "call")
    iOp1(optype::function, true)
    iOp2(optype::tuple, true)
iEndH

iBeginH(flow, CallResult, "call")
    iTarget(optype::any)
    iOp1(optype::function, true)
    iOp2(optype::tuple, true)
iEndH

iBeginH(flow, CallCallableResult, "call")
    iTarget(optype::any)
    iOp1(optype::refCallable, true)
iEndH

iBeginH(flow, CallCallableVoid, "call")
    iOp1(optype::refCallable, true)
iEndH

iBeginH(flow, Yield, "yield")
iEndH

iBeginH(flow, YieldUntil, "yield.until")
    iOp1(optype::any, true)
iEndH

iBeginH(flow, IfElse, "if.else")
    iTerminator()
    iOp1(optype::boolean, true)
    iOp2(optype::label, true)
    iOp3(optype::label, true)
iEndH

iBeginH(flow, Jump, "jump")
    iTerminator()
    iOp1(optype::label, true)
iEndH

iBeginH(flow, Switch, "switch")
    iTerminator()
    iOp1(optype::any, true)
    iOp2(optype::label, true)
    iOp3(optype::tuple, true)
iEndH
