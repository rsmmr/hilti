# $Id$
#
# Code generator for integer instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.convertToLLVM(type.IntegerType)
def _(type):
    return llvm.core.Type.int(type.width())

@codegen.when(instructions.integer.Add)
def _(self, i):
    op1 = self.llvmOp(i.op1(), "op1")
    op2 = self.llvmOp(i.op2(), "op2")
    result = self.builder().add(op1, op2, "tmp")
    self.llvmStoreInTarget(i.target(), result, "add")

@codegen.when(instructions.integer.Sub)
def _(self, i):
    op1 = self.llvmOp(i.op1(), "op1")
    op2 = self.llvmOp(i.op2(), "op2")
    result = self.builder().sub(op1, op2, "tmp")
    self.llvmStoreInTarget(i.target(), result, "sub")

@codegen.when(instructions.integer.Mult)
def _(self, i):
    op1 = self.llvmOp(i.op1(), "op1")
    op2 = self.llvmOp(i.op2(), "op2")
    result = self.builder().mult(op1, op2, "tmp")
    self.llvmStoreInTarget(i.target(), result, "mult")

@codegen.when(instructions.integer.Div)
def _(self, i):
    op1 = self.llvmOp(i.op1(), "op1")
    op2 = self.llvmOp(i.op2(), "op2")
    
    block_ok = self.llvmNewBlock("ok")
    block_exc = self.llvmNewBlock("exc")

    iszero = self.builder().icmp(llvm.core.IPRED_NE, op2, self.llvmConstInt(0), "tmp")
    self.builder().cbranch(iszero, block_ok, block_exc)
    
    self.pushBuilder(llvm.core.Builder.new(block_exc))
    self.llvmRaiseException("__hilti_exception_division_by_zero") 
    self.popBuilder()
    
    self.pushBuilder(llvm.core.Builder.new(block_ok))
    result = self.builder().sdiv(op1, op2, "tmp")
    self.llvmStoreInTarget(i.target(), result, "div")

    # Leave ok-builder for subsequent code. 
    
@codegen.when(instructions.integer.Eq)
def _(self, i):
    op1 = self.llvmOp(i.op1(), "op1")
    op2 = self.llvmOp(i.op2(), "op2")
    result = self.builder().icmp(llvm.core.IPRED_EQ, op1, op2, "tmp")
    self.llvmStoreInTarget(i.target(), result, "eq")

@codegen.when(instructions.integer.Lt)
def _(self, i):
    op1 = self.llvmOp(i.op1(), "op1")
    op2 = self.llvmOp(i.op2(), "op2")
    result = self.builder().icmp(llvm.core.IPRED_SLT, op1, op2, "tmp")
    self.llvmStoreInTarget(i.target(), result, "lt")

@codegen.when(instructions.integer.Ext)
def _(self, i):
    op1 = self.llvmOp(i.op1(), "op1")
    width = i.op2()
    assert width >= op1.type().width()
    
    result = self.builder().sext(op1, self.llvmTypeConvert(type.Integer(width)), "ext")
    self.llvmStoreInTarget(i.target(), result, "ext")
    
@codegen.when(instructions.integer.Ext)
def _(self, i):
    op1 = self.llvmOp(i.op1(), "op1")
    width = i.op2()
    assert width <= op1.type().width()
    
    result = self.builder().strunc(op1, self.llvmTypeConvert(type.Integer(width)), "trunc")
    self.llvmStoreInTarget(i.target(), result, "ext")
        
