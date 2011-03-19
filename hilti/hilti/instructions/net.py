# $Id$
"""
.. hlt:type:: net

   The ``net`` data type represents ranges of IP address, specified in CIDR
   notation. It transparently handles both IPv4 and IPv6 networks.
"""

import socket
import struct

import llvm.core

import llvm.core

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("net", 17, c="hlt_net")
class Net(type.ValueType, type.Constable, type.Classifiable):
    """Type for booleans."""
    def __init__(self, location=None):
        super(Net, self).__init__(location=location)

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        return llvm.core.Type.struct([llvm.core.Type.int(64)] * 2 + [llvm.core.Type.int(8)])

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::net_to_string";
        return typeinfo

    def llvmDefault(self, cg):
        """A ``network`` is initialized to ``::0/0``."""
        return llvm.core.Constant.struct([cg.llvmConstInt(0, 64)] * 2 + [cg.llvmConstInt(0, 8)])

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        """Network constants are specified in standard CIDR notation for
        either v4 addresses ("dotted quad", e.g., ``192.168.1.0/24``), or v6
        addresses (colon-notation, e.g., ``2001:db8::1428:57ab/48``)."""
        # Todo.
        pass

    def llvmConstant(self, cg, const):
        t = const.value()

        len = t[2]
        a1 = long(t[0]) & (0xFFFFFFFFFFFFFFFF << 64 - (min(64, len)))
        a2 = long(t[1]) & (0xFFFFFFFFFFFFFFFF << 64 - (len-64))

        net = [cg.llvmConstInt(a1, 64), cg.llvmConstInt(a2, 64)]
        len = [cg.llvmConstInt(len, 8)]
        return llvm.core.Constant.struct(net + len)

    def outputConstant(self, printer, const):
        (b1, b2, mask) = const.value()

        if b1 == 0 and b2 & 0xffffffff00000000 == 0:
            # IPv4
            net = "%s/%d" % (socket.inet_ntop(socket.AF_INET, struct.pack("!1L", b2)), mask - 96)
        else:
            # IPv6
            net = "%s/%d" % (socket.inet_ntop(socket.AF_INET6, struct.pack("!2Q", b1, b2)), mask)

        printer.output(net)

    ### Overidden from Classifiable

    def llvmToField(self, cg, ty, llvm_val):
        a1 = cg.llvmExtractValue(llvm_val, 0)
        a1 = cg.llvmHtoN(a1)
        a2 = cg.llvmExtractValue(llvm_val, 1)
        a2 = cg.llvmHtoN(a2)

        tmp = cg.llvmAlloca(llvm_val.type)
        addr = cg.builder().gep(tmp, [cg.llvmGEPIdx(0), cg.llvmGEPIdx(0)])
        cg.llvmAssign(a1, addr)

        addr = cg.builder().gep(tmp, [cg.llvmGEPIdx(0), cg.llvmGEPIdx(1)])
        cg.llvmAssign(a2, addr)

        len = cg.llvmSizeOf(self.llvmType(cg).elements[0])
        len = cg.builder().add(len, cg.llvmSizeOf(self.llvmType(cg).elements[1]))
        bits = cg.llvmExtractValue(llvm_val, 2)
        bits = cg.builder().zext(bits, llvm.core.Type.int(64))
        return type.Classifier.llvmClassifierField(cg, tmp, len, bits)

@hlt.overload(Equal, op1=cNet, op2=cNet, target=cBool)
class EqualNet(Operator):
    """ Returns True if the networks in *op1* and *op2* cover the same IP
    range. Note that an IPv4 network ``a.b.c.d/n`` will match the corresponding
    IPv6 ``::a.b.c.d/n``.

    Todo: Are the v6 vs v4 matching semantics right? Should it be
    ``::ffff:a.b.c.d``? Should v4 never match a v6 address?``
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

        builder = cg.builder()
        v1 = cg.llvmExtractValue(op1, 2)
        v2 = cg.llvmExtractValue(op2, 2)
        cmp3 = builder.icmp(llvm.core.IPRED_EQ, v1, v2)

        result1 = builder.and_(cmp1, cmp2)
        result2 = builder.and_(cmp1, cmp3)
        result = builder.and_(result1, result2)

        cg.llvmStoreInTarget(self, result)

@hlt.overload(Equal, op1=cNet, op2=cAddr, target=cBool)
class EqualAddr(Operator):
    """Returns True if the addr *op2* is part of the network *op1*."""
    def _codegen(self, cg):
        net = cg.llvmOp(self.op1())
        len = cg.llvmExtractValue(net, 2)
        addr = _maskAddr(cg, cg.llvmOp(self.op2()), len)

        builder = cg.builder()
        v1 = cg.llvmExtractValue(net, 0)
        v2 = cg.llvmExtractValue(addr, 0)
        cmp1 = builder.icmp(llvm.core.IPRED_EQ, v1, v2)

        builder = cg.builder()
        v1 = cg.llvmExtractValue(net, 1)
        v2 = cg.llvmExtractValue(addr, 1)
        cmp2 = builder.icmp(llvm.core.IPRED_EQ, v1, v2)

        cg.llvmStoreInTarget(self, builder.and_(cmp1, cmp2))

@hlt.instruction("net.family", op1=cNet, target=cEnum)
class Family(Instruction):
    """
    Returns the address family of *op1*, which can currently be either
    ``AddrFamily::IPv4`` or ``AddrFamiliy::IPv6``.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        v4 = cg.llvmEnumLabel("Hilti::AddrFamily::IPv4")
        v6 = cg.llvmEnumLabel("Hilti::AddrFamily::IPv6")
        result = cg.builder().select(_isv4(cg, op1), v4, v6)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("net.prefix", op1=cNet, target=cAddr)
class Prefix(Instruction):
    """
    Returns the network prefix as a masked IP addr.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        a = llvm.core.Constant.struct([cg.llvmConstInt(255, 64)] * 2)
        a = cg.llvmInsertValue(a, 0, cg.llvmExtractValue(op1, 0))
        a = cg.llvmInsertValue(a, 1, cg.llvmExtractValue(op1, 1))
        cg.llvmStoreInTarget(self, a)


@hlt.instruction("net.length", op1=cNet, target=cIntegerOfWidth(8))
class Length(Instruction):
    """
    Returns the length of the network prefix.
    """
    def _codegen(self, cg):
        # For IPv4, we need to subtract 96.
        op1 = cg.llvmOp(self.op1())
        len = cg.llvmExtractValue(op1, 2)
        lenv4 = cg.builder().sub(len, cg.llvmConstInt(96, 8))
        result = cg.builder().select(_isv4(cg, op1), lenv4, len)
        cg.llvmStoreInTarget(self, result)


# Returns an LLVM bool which is true for v4 and false otherwise.
def _isv4(cg, n):
    builder = cg.builder()
    v = cg.llvmExtractValue(n, 0)
    cmp1 = builder.icmp(llvm.core.IPRED_EQ, v, cg.llvmConstInt(0, 64))

    v = cg.llvmExtractValue(n, 1)
    masked = cg.builder().and_(v, cg.llvmConstInt(0xffffffff00000000, 64))
    cmp2 = cg.builder().icmp(llvm.core.IPRED_EQ, masked, cg.llvmConstInt(0, 64))

    v = cg.llvmExtractValue(n, 2)
    cmp3 = cg.builder().icmp(llvm.core.IPRED_UGE, v, cg.llvmConstInt(96, 8))

    return builder.and_(builder.and_(cmp1, cmp2), cmp3)

def _maskAddr(cg, addr, len):
    allbits = cg.llvmConstInt(0xFFFFFFFFFFFFFFFF, 64)
    zero = cg.llvmConstInt(0, 64)
    sixtyfour = cg.llvmConstInt(64, 64)
    onetwentyeight = cg.llvmConstInt(128, 64)
    len = cg.builder().sext(len, llvm.core.Type.int(64))

    a1 = cg.llvmExtractValue(addr, 0)
    shift = cg.builder().sub(sixtyfour, len)
    mask = cg.builder().shl(allbits, shift)
    cmp = cg.builder().icmp(llvm.core.IPRED_ULT, len, sixtyfour)
    mask = cg.builder().select(cmp, mask, allbits)
    a1 = cg.builder().and_(a1, mask)

    # Need to special case length zero because the shift isn't defined for
    # shifting 64 bits.
    cmp = cg.builder().icmp(llvm.core.IPRED_NE, len, zero)
    a1 = cg.builder().select(cmp, a1, zero)

    addr = cg.llvmInsertValue(addr, 0, a1)

    a2 = cg.llvmExtractValue(addr, 1)
    shift = cg.builder().sub(onetwentyeight, len)
    mask = cg.builder().shl(allbits, shift)
    cmp = cg.builder().icmp(llvm.core.IPRED_UGT, len, sixtyfour)
    mask = cg.builder().select(cmp, mask, zero)
    addr = cg.llvmInsertValue(addr, 1, cg.builder().and_(a2, mask))
    return addr


