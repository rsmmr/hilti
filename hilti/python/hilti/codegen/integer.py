# $Id$
#
# Code generator for integer instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
An ``int<n>`` is mapped to C integers depending on its width *n*, per the
following table: 
    
    ======  =======
    Width   C type
    ------  -------
    1..8    int8_t
    9..16   int16_t
    17..32  int32_t
    33..64  int64_t
    ======  =======
"""

@codegen.typeInfo(type.Integer)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.to_string = "__Hlt::int_to_string";
    typeinfo.to_int64 = "__Hlt::int_to_int64";
    
    for (w, p) in [(8, "int8_t"), (16, "int16_t"), (32, "int32_t"), (64, "int64_t")]:
        if type.width() <= w:
            typeinfo.c_prototype = p
            break
    else:
        assert False
    
    return typeinfo

@codegen.llvmDefaultValue(type.Integer)
def _(type):
    return codegen.llvmConstInt(0, type.width())

@codegen.llvmCtorExpr(type.Integer)
def _(op, refine_to):
    width = op.type().width()
    if not width and refine_to and refine_to.width():
        width = refine_to.width()
        
    return codegen.llvmConstInt(op.value(), width)
    
@codegen.llvmType(type.Integer)
def _(type, refine_to):
    return llvm.core.Type.int(type.width())

@codegen.operator(type.Integer, instructions.operators.Incr)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmConstInt(1, i.op1().type().width())
    result = self.builder().add(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.operator(type.Integer, instructions.operators.Equal)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
    self.llvmStoreInTarget(i.target(), result)
    
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
        
