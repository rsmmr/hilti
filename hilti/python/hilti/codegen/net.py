# $Id$
#
# Code generator for net instructions.
#
# Note: We depend on storing only the relevant bits for each prefix, and setting
# all others to zero. For constants, llvmCtorExpr() below takes care of that,
# and at the moment that's the only way to create instances of net. If other
# are added, however, we need to make sure to enforce this property. 

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = _doc_c_conversion = """An ``net`` is mapped to a ``__hlt_net``."""

def _llvmNetType():
    return llvm.core.Type.struct([llvm.core.Type.int(64)] * 2 + [llvm.core.Type.int(8)])

@codegen.typeInfo(type.Net)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "__hlt_net";
    typeinfo.to_string = "__Hlt::net_to_string";
    return typeinfo

@codegen.llvmDefaultValue(type.Net)
def _(type):
    return llvm.core.Constant.struct([codegen.llvmConstInt(0, 64)] * 2 + [codegen.llvmConstInt(0, 8)])

@codegen.llvmCtorExpr(type.Net)
def _(op, refine_to):
    t = op.value()
    
    len = t[2]
    a1 = long(t[0]) & (0xFFFFFFFFFFFFFFFF << 64 - (min(64, len)))
    a2 = long(t[1]) & (0xFFFFFFFFFFFFFFFF << 64 - (len-64))
    
    net = [codegen.llvmConstInt(a1, 64), codegen.llvmConstInt(a2, 64)]    
    len = [codegen.llvmConstInt(len, 8)]
    return llvm.core.Constant.struct(net + len)

@codegen.llvmType(type.Net)
def _(type, refine_to):
    return _llvmNetType()

@codegen.operator(instructions.net.EqualNet)
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
    
    builder = self.builder()
    v1 = self.llvmExtractValue(op1, 2)
    v2 = self.llvmExtractValue(op2, 2)
    cmp3 = builder.icmp(llvm.core.IPRED_EQ, v1, v2)
    
    result1 = builder.and_(cmp1, cmp2)
    result2 = builder.and_(cmp1, cmp3)
    result = builder.and_(result1, result2)
    
    self.llvmStoreInTarget(i.target(), result)


def _maskAddr(addr, len):
    allbits = codegen.llvmConstInt(0xFFFFFFFFFFFFFFFF, 64)
    zero = codegen.llvmConstInt(0, 64)
    sixtyfour = codegen.llvmConstInt(64, 64)
    onetwentyeight = codegen.llvmConstInt(128, 64)
    len = codegen.builder().sext(len, llvm.core.Type.int(64))
    
    a1 = codegen.llvmExtractValue(addr, 0)
    shift = codegen.builder().sub(sixtyfour, len)
    mask = codegen.builder().shl(allbits, shift)
    cmp = codegen.builder().icmp(llvm.core.IPRED_ULT, len, sixtyfour)
    mask = codegen.builder().select(cmp, mask, allbits)
    a1 = codegen.builder().and_(a1, mask)

    # Need to special case length zero because the shift isn't defined for
    # shifting 64 bits.
    cmp = codegen.builder().icmp(llvm.core.IPRED_NE, len, zero)
    a1 = codegen.builder().select(cmp, a1, zero)

    addr = codegen.llvmInsertValue(addr, 0, a1)
    
    a2 = codegen.llvmExtractValue(addr, 1)
    shift = codegen.builder().sub(onetwentyeight, len)
    mask = codegen.builder().shl(allbits, shift)
    cmp = codegen.builder().icmp(llvm.core.IPRED_UGT, len, sixtyfour)
    mask = codegen.builder().select(cmp, mask, zero)
    addr = codegen.llvmInsertValue(addr, 1, codegen.builder().and_(a2, mask))

    return addr
    
@codegen.operator(instructions.net.EqualAddr)
def _(self, i):
    net = self.llvmOp(i.op1())
    len = codegen.llvmExtractValue(net, 2)
    addr = _maskAddr(self.llvmOp(i.op2()), len)
    
    builder = self.builder()
    v1 = self.llvmExtractValue(net, 0)
    v2 = self.llvmExtractValue(addr, 0)
    cmp1 = builder.icmp(llvm.core.IPRED_EQ, v1, v2)
    
    builder = self.builder()
    v1 = self.llvmExtractValue(net, 1)
    v2 = self.llvmExtractValue(addr, 1)
    cmp2 = builder.icmp(llvm.core.IPRED_EQ, v1, v2)
    
    self.llvmStoreInTarget(i.target(), builder.and_(cmp1, cmp2))
    
# Returns an LLVM bool which is true for v4 and false otherwise.
def _isv4(n):
    builder = codegen.builder()
    v = codegen.llvmExtractValue(n, 0)
    cmp1 = builder.icmp(llvm.core.IPRED_EQ, v, codegen.llvmConstInt(0, 64))
    
    v = codegen.llvmExtractValue(n, 1)
    masked = codegen.builder().and_(v, codegen.llvmConstInt(0xffffffff00000000, 64))
    cmp2 = codegen.builder().icmp(llvm.core.IPRED_EQ, masked, codegen.llvmConstInt(0, 64))
    
    v = codegen.llvmExtractValue(n, 2)
    cmp3 = codegen.builder().icmp(llvm.core.IPRED_UGE, v, codegen.llvmConstInt(96, 8))
    
    return builder.and_(builder.and_(cmp1, cmp2), cmp3)
    
@codegen.when(instructions.net.Family)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    v4 = self.llvmEnumLabel("Hilti::AddrFamily::IPv4")
    v6 = self.llvmEnumLabel("Hilti::AddrFamily::IPv6")
    result = self.builder().select(_isv4(op1), v4, v6)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.net.Prefix)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    a = llvm.core.Constant.struct([self.llvmConstInt(255, 64)] * 2)
    a = self.llvmInsertValue(a, 0, self.llvmExtractValue(op1, 0))
    a = self.llvmInsertValue(a, 1, self.llvmExtractValue(op1, 1))
    self.llvmStoreInTarget(i.target(), a)

@codegen.when(instructions.net.Length)
def _(self, i):
    # For IPv4, we need to subtract 96.
    op1 = self.llvmOp(i.op1())
    len = codegen.llvmExtractValue(op1, 2)
    lenv4 = codegen.builder().sub(len, codegen.llvmConstInt(96, 8))
    result = codegen.builder().select(_isv4(op1), lenv4, len)
    self.llvmStoreInTarget(i.target(), result)

    
