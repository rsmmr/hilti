# $Id$
"""
Port numbers 
~~~~~~~~~~~~

The *port* data type represents TCP and UDP port numbers. Port constants are
written with a ``/tcp`` or ``/udp`` prefix, respectively. For example,
``80/tcp`` and ``53/udp``. If not explicitly initialized, a port ``0/tcp``.
"""

import llvm.core

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("port", 13)
class Port(type.ValueType, type.Constable, type.Unpackable):
    """Type for booleans."""
    def __init__(self, location=None):
        super(Port, self).__init__(location=location)

    # These values must match with those in libhilti/port.h
    _protos = { "tcp": 1, "udp": 2 }
        
    ### Overridden from HiltiType.
    
    def llvmType(self, cg):
        """A ``port`` is mapped to a ``hlt_port``."""
        return llvm.core.Type.struct([llvm.core.Type.int(16), llvm.core.Type.int(8)])

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_port";
        typeinfo.to_string = "hlt::port_to_string";
        typeinfo.to_int64 = "hlt::port_to_int64";
        return typeinfo
    
    def llvmDefault(self, cg):
        """A ``port`` is initialized to ``0/tcp``."""
        return llvm.core.Constant.struct([cg.llvmConstInt(0, 16), cg.llvmConstInt(Port._protos["tcp"], 8)])

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        if not isinstance(const.value(), str):
            util.internal_error("port constant must be a Python string")

        try:
            (port, proto) = const.value().split("/")
            port = int(port)
            proto = Port._protos[proto]
        except:
            util.internal_error("llvmCtorExpr: %s is not a port constant" % const.value())
            
        if port < 0 or port > 65535:
            vld.error("port constant is out of range")
    
    def llvmConstant(self, cg, const):
        (port, proto) = const.value().split("/")
        port = int(port)
        proto = Port._protos[proto]
            
        return llvm.core.Constant.struct([cg.llvmConstInt(port, 16), cg.llvmConstInt(proto, 8)])
        
    def outputConstant(self, printer, const):
        printer.output(const.value())
    
    ### Overridden from Unpackable.

    def formats(self, mod):
        return [
            ("PortTCP", None, False, """16-bit TCP port stored in host byte order."""), 
            ("PortUDP", None, False, """16-bit UDP port stored in host byte order."""), 
            ("PortTCPNetwork", None, False, """16-bit TCP port stored in network byte order."""), 
            ("PortUDPNetwork", None, False, """16-bit UDP port stored in network byte order.""")
            ]
        
    def llvmUnpack(self, cg, begin, end, fmt, arg):
        port = cg.llvmAlloca(self.llvmType(cg))
        iter = cg.llvmAlloca(cg.llvmType(type.IteratorBytes()))
        
        def unpackPort(nbo, protocol):
            def _unpackPort(case):
                fmt = "Int16Big" if nbo else "Int16"
                (val, i) = cg.llvmUnpack(type.Integer(16), begin, end, fmt)
                builder = cg.builder()
                p = builder.gep(port, [cg.llvmGEPIdx(0), cg.llvmGEPIdx(0)])
                cg.llvmAssign(val, p)
                proto = builder.gep(port, [cg.llvmGEPIdx(0), cg.llvmGEPIdx(1)])
                cg.llvmAssign(cg.llvmConstInt(Port._protos[protocol], 8), proto)
                cg.llvmAssign(i, iter)
                
            return _unpackPort
            
        cases = [
            ("Hilti::Packed::PortTCP", unpackPort(False, "tcp")),
            ("Hilti::Packed::PortUDP", unpackPort(False, "udp")),
            ("Hilti::Packed::PortTCPNetwork", unpackPort(True, "tcp")),
            ("Hilti::Packed::PortUDPNetwork", unpackPort(True, "udp"))
            ]
    
        cg.llvmSwitch(fmt, cases)
        return (cg.builder().load(port), cg.builder().load(iter))
    
@hlt.overload(Equal, op1=cPort, op2=cPort, target=cBool)
class Equal(Operator):
    """Returns True if the port in *op1* equals the port in *op2*. Note that
    a TCP port will *not* match the UDP port of the same value.
    """
    def codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        
        builder = cg.builder()
        v1 = cg.llvmExtractValue(op1, 0)
        v2 = cg.llvmExtractValue(op2, 0)
        cmp1 = builder.icmp(llvm.core.IPRED_EQ, v1, v2)
        
        builder = cg.builder()
        v1 = cg.llvmExtractValue(op1, 1)
        v2 = cg.llvmExtractValue(op2, 1)
        cmp2 = builder.icmp(llvm.core.IPRED_EQ, v1, v2)
        
        result = builder.and_(cmp1, cmp2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("port.protocol", op1=cPort, target=cEnum)
class Protocol(Instruction):
    """
    Returns the protocol of *op1*, which can currently be either
    ``Port::TCP`` or ``Port::UDP``. 
    """        
    def codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        
        builder = cg.builder()
        v = cg.llvmExtractValue(op1, 1)
    
        cmp = builder.icmp(llvm.core.IPRED_EQ, v, cg.llvmConstInt(Port._protos["tcp"], 8))
        
        tcp = cg.llvmEnumLabel("Hilti::Protocol::TCP")
        udp = cg.llvmEnumLabel("Hilti::Protocol::UDP")
        result = cg.builder().select(cmp, tcp, udp)
        
        cg.llvmStoreInTarget(self, result)
        
