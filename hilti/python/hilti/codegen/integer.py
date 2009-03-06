# $Id$
#
# Code generator for integer instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.makeTypeInfo(type.Integer)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.libhilti_fmt = "__Hlt::int_fmt";
    return typeinfo

@codegen.convertConstToLLVM(type.Integer)
def _(op):
    return llvm.core.Constant.int(llvm.core.Type.int(op.type().width()), op.value())

@codegen.convertTypeToLLVM(type.Integer)
def _(type):
    return llvm.core.Type.int(type.width())

@codegen.when(instructions.integer.Add)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().add(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.integer.Sub)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().sub(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.integer.Mul)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().mul(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.integer.Div)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())

    block_ok = self.llvmNewBlock("ok")
    block_exc = self.llvmNewBlock("exc")

    iszero = self.builder().icmp(llvm.core.IPRED_NE, op2, self.llvmConstInt(0, i.op2().type().width()))
    self.builder().cbranch(iszero, block_ok, block_exc)
    
    self.pushBuilder(block_exc)
    self.llvmRaiseExceptionByName("__hlt_exception_division_by_zero") 
    self.popBuilder()
    
    self.pushBuilder(block_ok)
    result = self.builder().sdiv(op1, op2)
    addr = self.llvmAddrLocalVar(self.currentFunction(), self.llvmCurrentFramePtr(), i.target().id().name())
    self.llvmAssign(result, addr)
    self.llvmStoreInTarget(i.target(), result)

    # Leave ok-builder for subsequent code. 
    
@codegen.when(instructions.integer.Eq)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.integer.Lt)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().icmp(llvm.core.IPRED_SLT, op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.integer.Ext)
def _(self, i):
    width = i.target().type().width()
    assert width >= i.op1().type().width()
    
    op1 = self.llvmOp(i.op1())
    
    result = self.builder().zext(op1, self.llvmTypeConvert(type.Integer(width)))
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.integer.Trunc)
def _(self, i):
    width = i.target().type().width()
    assert width <= i.op1().type().width()
    
    op1 = self.llvmOp(i.op1())
    
    result = self.builder().trunc(op1, self.llvmTypeConvert(type.Integer(width)))
    self.llvmStoreInTarget(i.target(), result)
        
