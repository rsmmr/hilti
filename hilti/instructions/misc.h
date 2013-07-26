
#include "define-instruction.h"

iBeginH(Misc, Select, "select")
    iTarget(optype::any)
    iOp1(optype::boolean, true)
    iOp2(optype::any, true)
    iOp3(optype::any, true)
iEndH

iBeginH(Misc, SelectValue, "select.value")
    iTarget(optype::any)
    iOp1(optype::any, true)
    iOp2(optype::tuple, true)
    iOp3(optype::optional(optype::any), true)
iEndH

iBeginH(Misc, Nop, "nop")
iEndH
