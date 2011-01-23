# $Id$
"""
.. hlt:type:: interval

    The ``interval`` data type represent a time period of certain length, with
    nanosecond resolution.

    Interval constants are specified in seconds, either as integers (``42``)
    or as floating poing values (``1295411800.123456789``). In the latter
    case, there may be at most 9 digits for the fractional part.

    If not further initialized, an ``interval`` instance is set to zero
    seconds.

    Note that when operating on ``interval`` values, behaviour in case of
    under- and overflow is undefined.

    Internally, intervals use a fixed-point represenation with 32 bit for full
    seconds, and 32 bit for the fraction of a second.
"""

# TODO: Much in this file is pretty much the same as for type Time. We could
# refactor that into common code. Not sure it's worth it though.

import llvm.core

from hilti.constraints import *
from hilti.instruction import *
from hilti.instructions.operators import *

@hlt.type("interval", 33, c="hlt_interval")
class Interval(type.ValueType, type.Constable):
    """Type for ``interval``."""
    def __init__(self):
        super(Interval, self).__init__()

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        """The 32 most-sigificant bits are representing seconds; the 32-bit
        least-signifant bits nanoseconds."""
        return llvm.core.Type.int(64)

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::interval_to_string";
        typeinfo.to_double = "hlt::interval_to_double";
        typeinfo.to_int64 = "hlt::interval_to_int64";
        return typeinfo

    def llvmDefault(self, cg):
        return cg.llvmConstInt(0, 64)

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        val = const.value()

        if not isinstance(val, tuple) or isinstance(val, list):
            return False

        if not len(val) == 2:
            return False

        return isinstance(val[0], int) and isinstance(val[1], int) and val[1] <= 1e10-1

    def llvmConstant(self, cg, const):
        (secs, nano) = const.value()

        return cg.llvmConstInt((secs << 32) + nano, 64)

    def outputConstant(self, printer, const):
        printer.output("%.9f" % const.value())

@hlt.constraint("int<32> | double")
def _int_or_double(ty, op, i):
    if isinstance(ty, type.Double):
        return (True, "")

    if isinstance(ty, type.Integer) and \
       ty.canCoerceTo(type.Integer(32)):
        return (True, "")

    return (False, "must be int<32> or double")

@hlt.instruction("interval.add", op1=cInterval, op2=cInterval, target=cInterval)
class Add(Instruction):
    """
    Adds intervals *op1* and *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().add(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("interval.sub", op1=cInterval, op2=cInterval, target=cInterval)
class Sub(Instruction):
    """
    Subtracts interval *op2* to the interval *op1*
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().sub(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("interval.lt",  op1=cInterval, op2=cInterval, target=cBool)
class Lt(Instruction):
    """
    Returns True if the interval represented by *op1* is earlier than that of *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().icmp(llvm.core.IPRED_SLT, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("interval.eq",  op1=cInterval, op2=cInterval, target=cBool)
class Eq(Instruction):
    """
    Returns True if the interval represented by *op1* equals that of *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Equal, op1=cInterval, op2=cInterval, target=cBool)
class Equal(Operator):
    """
    Returns True if the interval represented by *op1* equals that of *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("interval.from_secs", op1=_int_or_double, target=cInterval)
class FromSecs(Instruction):
    """Converts a value giving number of seconds in *op1* into a HILTI
    interval.
    """
    def _codegen(self, cg):
        if isinstance(self.op1().type(), type.Double):
            op1 = cg.llvmOp(self.op1())
            secs = cg.builder().fptoui(op1, llvm.core.Type.int(32))
            nanos = cg.builder().fsub(op1, cg.builder().uitofp(secs, llvm.core.Type.double()))
            nanos = cg.builder().fmul(nanos, llvm.core.Constant.real(llvm.core.Type.double(), "1e9"))
            nanos = cg.builder().fptoui(nanos, llvm.core.Type.int(32))

            secs = cg.builder().zext(secs, llvm.core.Type.int(64))
            nanos = cg.builder().zext(nanos, llvm.core.Type.int(64))

            result = cg.builder().shl(secs, cg.llvmConstInt(32, 64))
            result = cg.builder().or_(result, nanos)

        else:
            op1 = self.op1().coerceTo(cg, type.Integer(32))
            op1 = cg.llvmOp(op1)
            op1 = cg.builder().zext(op1, llvm.core.Type.int(64))
            result = cg.builder().shl(op1, cg.llvmConstInt(32, 64))

        cg.llvmStoreInTarget(self, result)

@hlt.instruction("interval.to_secs", op1=cInterval, target=_int_or_double)
class ToUnix(Instruction):
    """Converts the interval *op1* into a value giving number of seconds."""
    def _codegen(self, cg):
        if isinstance(self.target().type(), type.Double):
            op1 = cg.llvmOp(self.op1())

            secs = cg.builder().lshr(op1, cg.llvmConstInt(32, 64))
            secs = cg.builder().uitofp(secs, llvm.core.Type.double())

            nanos = cg.builder().and_(op1, cg.llvmConstInt(0xffffffff, 64))
            nanos = cg.builder().uitofp(nanos, llvm.core.Type.double())
            nanos = cg.builder().fdiv(nanos, llvm.core.Constant.real(llvm.core.Type.double(), "1e9"))

            result = cg.builder().fadd(secs, nanos)

        else:
            op1 = cg.llvmOp(self.op1())
            result = cg.builder().lshr(op1, cg.llvmConstInt(32, 64))
            result = cg.builder().trunc(result, llvm.core.Type.int(32))

        cg.llvmStoreInTarget(self, result)

