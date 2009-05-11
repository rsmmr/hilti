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

@codegen.operator(instructions.operators.GenericUnpack)
def _(self, i):
   op1 = self.llvmOp(i.op1()) 
   op2 = self.llvmOp(i.op2()) 
   t = i.target().type().types()[0]
   (val, iter) = self.llvmUnpack(t, op1, op2, i.op3())
   self.llvmStoreTupleInTarget(i.target(), (val, iter))
