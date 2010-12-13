# $Id$
"""
XXX
"""

_doc_section = ("misc", "Miscellaneous")

import llvm.core

from hilti import hlt
from hilti import type
from hilti.constraints import *
from hilti.instruction import *

@hlt.instruction("nop")
class Nop(Instruction):
    """
    Evaluates into nothing.
    """
    def _codegen(self, cg):
        pass

@hlt.instruction("select", op1=cBool, op2=cAny, op3=cSameTypeAsOp(2), target=cSameTypeAsOp(2))
class Select(Instruction):
    """Returns *op2* if *op1* is True, and *op3* otherwise."""
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        op3 = cg.llvmOp(self.op3())
        select = cg.builder().select(op1, op2, op3)
        cg.llvmStoreInTarget(self, select)

