# $Id$
"""
.. hlt:type:: time

    The ``time`` data type represents specific points in time, with nanosecond
    resolution. The earliest time that can be represented is the standard UNIX
    epoch, i.e., Jan 1 1970 UTC.

    Time constants are specified in seconds since the epoch, either as
    integers (``1295411800``) or as floating poing values
    (``1295411800.123456789``). In the latter case, there may be at most 9
    digits for the fractional part.

    A special constant ``Epoch`` is provided as well.

    If not further initialized, a ``time`` instance is set to the ``Epoch``.

    Note that when operating on time values, behaviour in case of under- and
    overflow is undefined.

    Internally, ``time`` uses a fixed-point represenation with 32 bit for full
    seconds, and 32 bit for the fraction of a second.
"""

import llvm.core

from hilti.constraints import *
from hilti.instruction import *
from hilti.instructions.operators import *

Epoch = 0

@hlt.type("time", 32, c="hlt_time")
class Time(type.ValueType, type.Constable):
    """Type for ``time``."""
    def __init__(self):
        super(Time, self).__init__()

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        """The 32 most-sigificant bits are representing seconds; the 32-bit
        least-signifant bits nanoseconds."""
        return llvm.core.Type.int(64)

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::time_to_string";
        typeinfo.to_double = "hlt::time_to_double";
        typeinfo.to_int64 = "hlt::time_to_int64";
        return typeinfo

    def llvmDefault(self, cg):
        return cg.llvmConstInt(Epoch, 64)

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
        printer.output("%.9f" % const.value())

@hlt.constraint("int<32> | double")
def _int_or_double(ty, op, i):
    if isinstance(ty, type.Double):
        return (True, "")

    if isinstance(ty, type.Integer) and \
       ty.canCoerceTo(type.Integer(32)):
        return (True, "")

    return (False, "must be int<32> or double")

@hlt.instruction("time.add", op1=cTime, op2=cInterval, target=cTime)
class Add(Instruction):
    """
    Adds interval *op2* to the time *op1*
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().add(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("time.sub", op1=cTime, op2=cInterval, target=cTime)
class Sub(Instruction):
    """
    Subtracts interval *op2* to the time *op1*
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().sub(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("time.lt",  op1=cTime, op2=cTime, target=cBool)
class Lt(Instruction):
    """
    Returns True if the time represented by *op1* is earlier than that of *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().icmp(llvm.core.IPRED_SLT, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("time.gt",  op1=cTime, op2=cTime, target=cBool)
class Gt(Instruction):
    """
    Returns True if the time represented by *op1* is later than that of *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().icmp(llvm.core.IPRED_SGT, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("time.eq",  op1=cTime, op2=cTime, target=cBool)
class Eq(Instruction):
    """
    Returns True if the time represented by *op1* equals that of *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Equal, op1=cTime, op2=cTime, target=cBool)
class Equal(Operator):
    """
    Returns True if the time represented by *op1* equals that of *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("time.wall",  op1=None, op2=None, target=cTime)
class Wall(Instruction):
    """
    Returns the current wall time (i.e., real-time). Note that the resolution
    of the returned value is OS dependent.
    """
    def _codegen(self, cg):
        wall = cg.llvmCallC("hlt::time_wall", [])
        cg.llvmStoreInTarget(self, wall)

@hlt.instruction("time.nsecs",  op1=cTime, op2=None, target=cIntegerOfWidth(64))
class Nsecs(Instruction):
    """
    Returns the time *op1* as nanoseconds since the epoch.
    """
    def _codegen(self, cg):
        nsecs = cg.llvmCallC("hlt::time_nsecs", [self.op1()])
        cg.llvmStoreInTarget(self, nsecs)

@hlt.instruction("time.as_int",  op1=cTime, op2=None, target=cIntegerOfWidth(64))
class AsInt(Instruction):
    """Returns *op1*' in seconds since the epoch, rounded down to the nearest
    integer.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        result = cg.builder().udiv(op1, cg.llvmConstInt(1000000000, 64))
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("time.as_double",  op1=cTime, op2=None, target=cDouble)
class AsDouble(Instruction):
    """Returns *op1* in seconds since the epoch, rounded down to the nearest
    value that can be represented as a double.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        val = cg.builder().uitofp(op1, llvm.core.Type.double())
        val = cg.builder().fdiv(val, llvm.core.Constant.real(llvm.core.Type.double(), "1e9"))
        cg.llvmStoreInTarget(self, val)

