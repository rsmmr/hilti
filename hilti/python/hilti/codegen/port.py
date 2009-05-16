# $Id$
#
# Code generator for addr instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = _doc_c_conversion = """A ``port`` is mapped to a ``__hlt_port``."""

# These values must match with those in libhilti/port.h
_protos = { "tcp": 1, "udp": 2 }

def _llvmPortType():
    return llvm.core.Type.struct([llvm.core.Type.int(16), llvm.core.Type.int(8)])

@codegen.typeInfo(type.Port)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "__hlt_port";
    typeinfo.to_string = "__Hlt::port_to_string";
    typeinfo.to_int64 = "__Hlt::port_to_int64";
    return typeinfo

@codegen.llvmDefaultValue(type.Port)
def _(type):
    return llvm.core.Constant.struct([codegen.llvmConstInt(0, 16), codegen.llvmConstInt(_protos["tcp"], 8)])

@codegen.llvmCtorExpr(type.Port)
def _(op, refine_to):
    try:
        (port, proto) = op.value().split("/")
        port = int(port)
        proto = _protos[proto]
    except:
        util.internal_error("llvmCtorExpr: %s is not a port constant" % op.value())
        
    return llvm.core.Constant.struct([codegen.llvmConstInt(port, 16), codegen.llvmConstInt(proto, 8)])

@codegen.llvmType(type.Port)
def _(type, refine_to):
    return _llvmPortType()

@codegen.unpack(type.Port)
def _(t, begin, end, fmt, arg):
    """Port unpacking supports the following formats:
    
    .. literalinclude:: /libhilti/hilti.hlt
       :start-after: %doc-packed-port-start
       :end-before:  %doc-packed-port-end
    """
    
    port = codegen.llvmAlloca(codegen.llvmTypeConvert(t))
    iter = codegen.llvmAlloca(codegen.llvmTypeConvert(type.IteratorBytes()))

    def unpackPort(nbo, protocol):
        def _unpackPort(case):
            fmt = "Hilti::Packed::Int16Big" if nbo else "Hilti::Packed::Int16"
            (val, i) = codegen.llvmUnpack(type.Integer(16), begin, end, fmt)
            builder = codegen.builder()
            p = builder.gep(port, [codegen.llvmGEPIdx(0), codegen.llvmGEPIdx(0)])
            codegen.llvmInit(val, p)
            proto = builder.gep(port, [codegen.llvmGEPIdx(0), codegen.llvmGEPIdx(1)])
            codegen.llvmInit(codegen.llvmConstInt(_protos[protocol], 8), proto)
            
        return _unpackPort
        
    cases = [
        ("Hilti::Packed::PortTCP", unpackPort(False, "tcp")),
        ("Hilti::Packed::PortUDP", unpackPort(False, "udp")),
        ("Hilti::Packed::PortTCPNetwork", unpackPort(True, "tcp")),
        ("Hilti::Packed::PortUDPNetwork", unpackPort(True, "udp"))
        ]

    codegen.llvmSwitch(fmt, cases)
    return (codegen.builder().load(port), codegen.builder().load(iter))

@codegen.operator(instructions.port.Equal)
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

@codegen.when(instructions.port.Protocol)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    
    builder = self.builder()
    v = codegen.llvmExtractValue(op1, 1)

    cmp = builder.icmp(llvm.core.IPRED_EQ, v, codegen.llvmConstInt(_protos["tcp"], 8))
    
    tcp = self.llvmEnumLabel("Hilti::Protocol::TCP")
    udp = self.llvmEnumLabel("Hilti::Protocol::UDP")
    result = self.builder().select(cmp, tcp, udp)
    
    self.llvmStoreInTarget(i.target(), result)
    
