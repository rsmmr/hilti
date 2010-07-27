# $Id$
"""
Select
~~~~~~

XXX
"""

import llvm.core

import hilti.function as function

from hilti.instruction import *
from hilti.constraints import *

@hlt.instruction("select", op1=cBool, op2=cAny, op3=cSameTypeAsOp(2), target=cSameTypeAsOp(2))
class Select(Instruction):
    """Returns *op2* if *op1* is True, and *op3* otherwise."""
    def codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        op3 = cg.llvmOp(self.op3())
        select = cg.builder().select(op1, op2, op3)
        cg.llvmStoreInTarget(self, select)
