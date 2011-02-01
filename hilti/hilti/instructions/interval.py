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
        return cg.llvmConstInt(secs * 1000000000 + nano, 64)

    def outputConstant(self, printer, const):
        secs = const.value()[0]
        frac = const.value()[1] / 1e9
        frac = "%.9f" % frac

        printer.output("interval(%d.%s)" % (secs, frac[2:]))

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

@hlt.instruction("interval.mul", op1=cInterval, op2=cIntegerOfWidth(64), target=cInterval)
class Sub(Instruction):
    """
    Subtracts interval *op2* to the interval *op1*
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().mul(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("interval.lt",  op1=cInterval, op2=cInterval, target=cBool)
class Lt(Instruction):
    """
    Returns True if the interval represented by *op1* is shorter than that of *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().icmp(llvm.core.IPRED_SLT, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("interval.gt",  op1=cInterval, op2=cInterval, target=cBool)
class Gt(Instruction):
    """
    Returns True if the interval represented by *op1* is larger than that of *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().icmp(llvm.core.IPRED_SGT, op1, op2)
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

@hlt.instruction("interval.nsecs",  op1=cInterval, op2=None, target=cIntegerOfWidth(64))
class Nsecs(Instruction):
    """
    Returns the interval *op1* as nanoseconds since the epoch.
    """
    def _codegen(self, cg):
        nsecs = cg.llvmCallC("hlt::interval_nsecs", [self.op1()])
        cg.llvmStoreInTarget(self, nsecs)

@hlt.instruction("interval.as_int",  op1=cInterval, op2=None, target=cIntegerOfWidth(64))
class AsInt(Instruction):
    """Returns *op1*' in seconds, rounded down to the nearest
    integer.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        result = cg.builder().udiv(op1, cg.llvmConstInt(1000000000, 64))
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("interval.as_double",  op1=cInterval, op2=None, target=cDouble)
class AsDouble(Instruction):
    """Returns *op1* in seconds, rounded down to the nearest
    value that can be represented as a double.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        val = cg.builder().uitofp(op1, llvm.core.Type.double())
        val = cg.builder().fdiv(val, llvm.core.Constant.real(llvm.core.Type.double(), "1e9"))
        cg.llvmStoreInTarget(self, val)

