# $Id$
#
# Code generator for addr instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = _doc_c_conversion = """An ``addr`` is mapped to a ``__hlt_addr``."""

@codegen.typeInfo(type.Addr)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "__hlt_addr";
    typeinfo.to_string = "__Hlt::addr_to_string";
    return typeinfo

@codegen.llvmDefaultValue(type.Addr)
def _(type):
    return llvm.core.Constant.struct([codegen.llvmConstInt(0, 64)] * 2)

@codegen.llvmCtorExpr(type.Addr)
def _(op, refine_to):
    a = llvm.core.Constant.struct([codegen.llvmConstInt(i, 64) for i in op.value()])
    return a

@codegen.llvmType(type.Addr)
def _(type, refine_to):
    return llvm.core.Type.struct([llvm.core.Type.int(64)] * 2)

@codegen.operator(instructions.addr.Equal)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    
    builder = self.builder()
    v1 = self.llvmExtractValue(op1, 0)
    v2 = self.llvmExtractValue(op2, 0)
    cmp1 = builder.icmp(llvm.core.IPRED_EQ, v1, v2)
    
    builder = self.builder()
    v1 = self.llvmExtractValue(op1, 1)
    v2 = self.llvmExtractValue(op2, 1)
    cmp2 = builder.icmp(llvm.core.IPRED_EQ, v1, v2)
    
    result = builder.and_(cmp1, cmp2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.operator(instructions.addr.Unpack)
def _(self, i):
    # XXX Not implemetned.
    pass

@codegen.when(instructions.addr.Family)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    
    builder = self.builder()
    v = codegen.llvmExtractValue(op1, 0)
    cmp1 = builder.icmp(llvm.core.IPRED_EQ, v, self.llvmConstInt(0, 64))
    
    v = codegen.llvmExtractValue(op1, 1)
    masked = self.builder().and_(v, self.llvmConstInt(0xffffffff00000000, 64))
    cmp2 = self.builder().icmp(llvm.core.IPRED_EQ, masked, self.llvmConstInt(0, 64))
    
    v4 = self.llvmEnumLabel("Hilti::AddrFamily::IPv4")
    v6 = self.llvmEnumLabel("Hilti::AddrFamily::IPv6")
    
    result = self.builder().select(builder.and_(cmp1, cmp2), v4, v6)
    self.llvmStoreInTarget(i.target(), result)
    
