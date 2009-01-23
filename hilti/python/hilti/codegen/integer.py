# $Id$
#
# Code generator for integer instructions.

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.when(instructions.integer.Add)
def _(self, i):
    op1 = self.llvmOp(i.op1(), "op1")
    op2 = self.llvmOp(i.op2(), "op2")
    result = self.builder().add(op1, op2, "tmp")
    self.llvmStoreInTarget(i.target(), result, "target")
