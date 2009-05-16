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
   
   if i.op3():
       op3 = self.llvmOp(i.op3())
   else:
       op3 = None
    
   begin = self.llvmExtractValue(op1, 0)
   end = self.llvmExtractValue(op1, 1)
   
   t = i.target().type().types()[0]
   (val, iter) = self.llvmUnpack(t, begin, end, i.op2(), op3)
   self.llvmStoreTupleInTarget(i.target(), (val, iter))
