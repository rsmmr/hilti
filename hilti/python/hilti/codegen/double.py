# $Id$
#
# Code generator for double instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.makeTypeInfo(type.Double)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.to_string = "__Hlt::double_to_string";
    typeinfo.to_double = "__Hlt::double_to_double";
    return typeinfo

@codegen.convertConstToLLVM(type.Double)
def _(op):
    return codegen.llvmConstDouble(op.value())

@codegen.convertTypeToLLVM(type.Double)
def _(type):
    return llvm.core.Type.double()

@codegen.convertTypeToC(type.Double)
def _(type):
    """A ``double`` is mapped transparently to a C double."""
    return codegen.llvmTypeConvert(type)

@codegen.when(instructions.double.Add)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().add(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.double.Sub)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().sub(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.double.Mul)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().mul(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.double.Div)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())

    block_ok = self.llvmNewBlock("ok")
    block_exc = self.llvmNewBlock("exc")

    iszero = self.builder().fcmp(llvm.core.RPRED_ONE, op2, self.llvmConstDouble(0))
    self.builder().cbranch(iszero, block_ok, block_exc)
    
    self.pushBuilder(block_exc)
    self.llvmRaiseExceptionByName("__hlt_exception_division_by_zero") 
    self.popBuilder()
    
    self.pushBuilder(block_ok)
    result = self.builder().fdiv(op1, op2)
    addr = self.llvmAddrLocalVar(self.currentFunction(), self.llvmCurrentFramePtr(), i.target().id().name())
    self.llvmAssign(result, addr)
    self.llvmStoreInTarget(i.target(), result)

    # Leave ok-builder for subsequent code. 
    
@codegen.when(instructions.double.Eq)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().fcmp(llvm.core.RPRED_OEQ, op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.double.Lt)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().fcmp(llvm.core.RPRED_OLT, op1, op2)
    self.llvmStoreInTarget(i.target(), result)
