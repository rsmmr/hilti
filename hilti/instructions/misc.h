
#include "instructions/define-instruction.h"

iBeginH(Misc, Select, "select")
    iTarget(optype::any)
    iOp1(optype::boolean, true)
    iOp2(optype::any, true)
    iOp3(optype::any, true)
iEndH

iBeginH(Misc, Nop, "nop")
iEndH
