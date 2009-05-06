# $Id$
#
# Code generator for operators.

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.when(instruction.Operator)
def _(self, i):
    self.llvmExecuteOperator(i)

@codegen.operator(instructions.operators.GenericAssign)
def _(self, i):
    op = self.llvmOp(i.op1(), i.target().type())
    self.llvmStoreInTarget(i.target(), op)
