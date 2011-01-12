# $Id$
"""
.. hlt:type:: addr Addresses

    The ``addr`` data type represents IP addresses, and it transparently
    handles both IPv4 and IPv6 addresses.

    Address constants are specified in the standard notation for IPv4
    addresses ("dotted quad", e.g., ``192.168.1.1``) or IPv6 addresses
    (colon-notation, e.g., ``2001:db8::1428:57ab``).

    If not further specified, instances of type ``addr`` are by default
    initialized to ``::0``.
"""

import socket
import struct

import llvm.core

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("addr", 12, c="hlt_addr", hdr="addr.h")
class Addr(type.ValueType, type.Constable, type.Unpackable):
    """Type for ``addr``."""
    def __init__(self, location=None):
        super(Addr, self).__init__(location=location)

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        return llvm.core.Type.struct([llvm.core.Type.int(64)] * 2)

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::addr_to_string";
        return typeinfo

    def llvmDefault(self, cg):
        return llvm.core.Constant.struct([cg.llvmConstInt(0, 64)] * 2)

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        # Todo: implement.
        pass

    def llvmConstant(self, cg, const):
        a = llvm.core.Constant.struct([cg.llvmConstInt(i, 64) for i in const.value()])
        return a

    def outputConstant(self, printer, const):
        (b1, b2) = const.value()

        if b1 == 0 and b2 & 0xffffffff00000000 == 0:
            # IPv4
            addr = socket.inet_ntop(socket.AF_INET, struct.pack("!1L", b2))
        else:
            # IPv6
            addr = socket.inet_ntop(socket.AF_INET6, struct.pack("!2Q", b1, b2))

        printer.output(addr)

    ### Overridden from Unpackable.

    def formats(self, mod):
        return [
            ("IPv4", None, False, """32-bit IPv4 address stored in host byte order."""),
            ("IPv6", None, False, """128-bit IPv6 address stored in host byte order."""),
            ("IPv4Network", None, False, """32-bit IPv4 address stored in network byte order."""),
            ("IPv6Network", None, False, """128-bit IPv6 address stored in network byte order.""")
            ]

    def llvmUnpack(self, cg, begin, end, fmt, arg):
        addr = cg.llvmAlloca(self.llvmType(cg))
        iter = cg.llvmAlloca(cg.llvmType(type.IteratorBytes()))

        def emitV4(nbo):
            def _emitV4(case):
                fmt = "Int32Big" if nbo else "Int32"
                (val, i) = cg.llvmUnpack(type.Integer(32), begin, end, fmt)
                builder = cg.builder()
                val = builder.zext(val, llvm.core.Type.int(64))
                a1 = builder.gep(addr, [cg.llvmGEPIdx(0), cg.llvmGEPIdx(0)])
                cg.llvmAssign(cg.llvmConstInt(0, 64), a1)
                a2 = builder.gep(addr, [cg.llvmGEPIdx(0), cg.llvmGEPIdx(1)])
                cg.llvmAssign(val, a2)
                cg.llvmAssign(i, iter)

            return _emitV4

        def emitV6(nbo):
            def _emitV6(case):
                fmt = "Int64Big" if nbo else "Int64"
                (val1, i) = cg.llvmUnpack(type.Integer(64), begin, end, fmt)
                (val2, i) = cg.llvmUnpack(type.Integer(64), i, end, fmt)

                builder = cg.builder()
                a1 = builder.gep(addr, [cg.llvmGEPIdx(0), cg.llvmGEPIdx(0)])
                a2 = builder.gep(addr, [cg.llvmGEPIdx(0), cg.llvmGEPIdx(1)])

                if nbo:
                    cg.llvmAssign(val1, a1)
                    cg.llvmAssign(val2, a2)
                else:
                    cg.llvmAssign(val1, a2)
                    cg.llvmAssign(val2, a1)

                cg.llvmAssign(i, iter)

            return _emitV6

        cases = [("Hilti::Packed::IPv4", emitV4(False)), ("Hilti::Packed::IPv4Network", emitV4(True)),
                 ("Hilti::Packed::IPv6", emitV6(False)), ("Hilti::Packed::IPv6Network", emitV6(True))]

        cg.llvmSwitch(fmt, cases)
        return (cg.builder().load(addr), cg.builder().load(iter))

@hlt.overload(Equal, op1=cAddr, op2=cAddr, target=cBool)
class Equal(Operator):
    """Returns True if the address *op1* equals the address *op2*. Note that
    an IPv4 address ``a.b.c.d`` will match the corresponding IPv6
    ``::a.b.c.d``.

    .. note:: Are the IPv6 vs IPv4 matching semantics right? Should it be ``::ffff:a.b.c.d``? Should IPv4 never match an IPv6 address?
    """
    def _codegen(self, cg):
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

@hlt.instruction("addr.family", op1=cAddr, target=cEnum)
class Family(Instruction):
    """Returns the address family of *op1*, which can be either
    :hlt:glob:`hilti::AddrFamily::IPv4` or
    :hlt:glob:`hilti::AddrFamily::IPv6`.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())

        builder = cg.builder()
        v = cg.llvmExtractValue(op1, 0)
        cmp1 = builder.icmp(llvm.core.IPRED_EQ, v, cg.llvmConstInt(0, 64))

        v = cg.llvmExtractValue(op1, 1)
        masked = cg.builder().and_(v, cg.llvmConstInt(0xffffffff00000000, 64))
        cmp2 = cg.builder().icmp(llvm.core.IPRED_EQ, masked, cg.llvmConstInt(0, 64))

        v4 = cg.llvmEnumLabel("Hilti::AddrFamily::IPv4")
        v6 = cg.llvmEnumLabel("Hilti::AddrFamily::IPv6")

        result = cg.builder().select(builder.and_(cmp1, cmp2), v4, v6)
        cg.llvmStoreInTarget(self, result)

