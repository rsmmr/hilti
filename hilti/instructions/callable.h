///
/// \type X
/// 
/// \ctor X
///
/// \cproto X
///

#include "define-instruction.h"

iBeginH(callable, NewFunction, "new")
    iTarget(optype::refCallable)
    iOp1(optype::typeCallable, true);
    iOp2(optype::function, true)
    iOp3(optype::tuple, false)
iEndH

iBeginH(callable, NewHook, "new")
    iTarget(optype::refCallable)
    iOp1(optype::typeCallable, true);
    iOp2(optype::hook, true)
    iOp3(optype::tuple, false)
iEndH
