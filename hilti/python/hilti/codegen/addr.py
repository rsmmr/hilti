# $Id$
#
# Code generator for addr instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = _doc_c_conversion = """An ``addr`` is mapped to a ``__hlt_addr``."""

def _llvmAddrType():
    return llvm.core.Type.struct([llvm.core.Type.int(64)] * 2)

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
    return _llvmAddrType()

@codegen.unpack(type.Addr)
def _(t, begin, end, fmt, arg):
    """Address unpacking supports the following formats:
    
    .. literalinclude:: /libhilti/hilti.hlt
       :start-after: %doc-packed-addr-start
       :end-before:  %doc-packed-addr-end
    """
    
    addr = codegen.llvmAlloca(codegen.llvmTypeConvert(t))
    iter = codegen.llvmAlloca(codegen.llvmTypeConvert(type.IteratorBytes(type.Bytes())))
    
    def emitV4(nbo):
        def _emitV4(case):
            fmt = "Hilti::Packed::Int32Big" if nbo else "Hilti::Packed::Int32"
            (val, i) = codegen.llvmUnpack(type.Integer(32), begin, end, fmt)
            builder = codegen.builder()
            val = builder.zext(val, llvm.core.Type.int(64))
            a1 = builder.gep(addr, [codegen.llvmGEPIdx(0), codegen.llvmGEPIdx(0)])
            codegen.llvmInit(codegen.llvmConstInt(0, 64), a1)
            a2 = builder.gep(addr, [codegen.llvmGEPIdx(0), codegen.llvmGEPIdx(1)])
            codegen.llvmInit(val, a2)
            codegen.llvmInit(i, iter)
            
        return _emitV4
    
    def emitV6(nbo):
        def _emitV6(case):
            fmt = "Hilti::Packed::Int64Big" if nbo else "Hilti::Packed::Int64"
            (val1, i) = codegen.llvmUnpack(type.Integer(64), begin, end, fmt)
            (val2, i) = codegen.llvmUnpack(type.Integer(64), i, end, fmt)
            
            builder = codegen.builder()
            a1 = builder.gep(addr, [codegen.llvmGEPIdx(0), codegen.llvmGEPIdx(0)])
            a2 = builder.gep(addr, [codegen.llvmGEPIdx(0), codegen.llvmGEPIdx(1)])
            
            if nbo:
                codegen.llvmInit(val1, a1)
                codegen.llvmInit(val2, a2)
            else:
                codegen.llvmInit(val1, a2)
                codegen.llvmInit(val2, a1)
            
            codegen.llvmInit(i, iter)
        
        return _emitV6

    cases = [("Hilti::Packed::IPv4", emitV4(False)), ("Hilti::Packed::IPv4Network", emitV4(True)), 
             ("Hilti::Packed::IPv6", emitV6(False)), ("Hilti::Packed::IPv6Network", emitV6(True))]
              
    codegen.llvmSwitch(fmt, cases)
    return (codegen.builder().load(addr), codegen.builder().load(iter))

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
    
