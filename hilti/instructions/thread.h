
#include "instructions/define-instruction.h"

iBeginH(thread, GetContext, "thread.get_context")
    iTarget(optype::refContext)
iEndH

iBeginH(thread, ThreadID, "thread.id")
    iTarget(optype::int64)
iEndH

iBeginH(thread, Schedule, "thread.schedule")
    iOp1(optype::function, true)
    iOp2(optype::tuple, true)
    iOp3(optype::optional(optype::int64), true)
iEndH

iBeginH(thread, SetContext, "thread.set_context")
    iOp1(optype::any, true)
iEndH

