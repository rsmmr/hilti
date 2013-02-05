
#include "instructions/define-instruction.h"

iBeginH(exception, New, "new")
    iTarget(optype::refException)
    iOp1(optype::typeException, true);
iEndH

iBeginH(exception, NewWithArg, "new")
    iTarget(optype::refException)
    iOp1(optype::typeException, true);
    iOp2(optype::any, true);
iEndH

iBeginH(exception, Throw, "exception.throw")
    iOp1(optype::refException, true)
iEndH

iBeginH(exception, __BeginHandler, "exception.__begin_handler")
    iOp1(optype::label, true)
    iOp2(optype::optional(optype::typeException), true);
iEndH

iBeginH(exception, __EndHandler, "exception.__end_handler")
iEndH

iBeginH(exception, __GetAndClear, "exception.__get_and_clear")
    iTarget(optype::refException)
iEndH

iBeginH(exception, __Clear, "exception.__clear")
iEndH

// iBeginH(exception, __Match, "exception.__match")
//     iTerminator()
//     iOp1(optype::typeException, true);
//     iOp2(optype::label, true)
//     iOp3(optype::label, true)
// iEndH
// 
// iBeginH(exception, __Current, "exception.__current")
//     iTarget(optype::refException)
// iEndH
// 
// iBeginH(exception, __Reraise, "exception.__reraise")
//     iTerminator()
//     iOp1(optype::typeException, true);
//     iOp2(optype::label, true)
//     iOp3(optype::label, true)
// iEndH
// #endif
// 
