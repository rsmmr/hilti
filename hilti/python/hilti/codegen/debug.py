# $Id$
#
# Code generator for debug instructions. 

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

import sys

@codegen.when(instructions.debug.Msg)
def _(self, i):
    if codegen.debugMode():
        self.llvmGenerateCCallByName("hlt::debug_printf", [i.op1(), i.op2(), i.op3()])
    
@codegen.when(instructions.debug.Assert)
def _(self, i):
    if codegen.debugMode():
        op1 = self.llvmOp(i.op1())
        block_true = self.llvmNewBlock("true")
        block_false = self.llvmNewBlock("false")

        self.builder().cbranch(op1, block_true, block_false)
        
        self.pushBuilder(block_false)
        arg = codegen.llvmOp(i.op2()) if i.op2() else string._makeLLVMString("")
        self.llvmRaiseExceptionByName("hlt_exception_assertion_error", i.location(), arg)
        self.popBuilder()

        self.pushBuilder(block_true)
        # Leave on stack. 
        
@codegen.when(instructions.debug.InternalError)
def _(self, i):
    arg = codegen.llvmOp(i.op1())
    self.llvmRaiseExceptionByName("hlt_exception_internal_error", i.location(), arg)
