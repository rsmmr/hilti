# $Id$
"""
.. hlt:type:: double

   The ``double`` data type represents a 64-bit floating-point numbers.
"""

import llvm.core

from hilti.constraints import *
from hilti.instruction import *

@hlt.type("double", 2, c="double")
class Double(type.ValueType, type.Constable):
    """Type for ``double``."""
    def __init__(self):
        super(Double, self).__init__()

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        return llvm.core.Type.double()

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::double_to_string";
        typeinfo.to_double = "hlt::double_to_double";
        return typeinfo

    def llvmDefault(self, cg):
        """A ``double`` is initially initialized to zero."""
        return cg.llvmConstDouble(0)

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        val = const.value()

        if isinstance(val, float):
            return True

        if not isinstance(val, list) and not isinstance(val, tuple):
            return False

        return isinstance(val[0], int) and isinstance(val[1], int)

    def _float(self, const):
        val = const.value()

        if not isinstance(val, float):
            if not val[1]:
                val = float(val[0])
            else:
                val = float(val[0]) + float(val[1]) / 1e9

        return val

    def llvmConstant(self, cg, const):
        return cg.llvmConstDouble(self._float(const))

    def outputConstant(self, printer, const):
        secs = const.value()[0]
        frac = const.value()[1] / 1e9
        frac = "%.9f" % frac
        printer.output("%d.%s" % (secs, frac[2:]))

@hlt.instruction("double.add", op1=cDouble, op2=cDouble, target=cDouble)
class Add(Instruction):
    """
    Calculates the sum of the two operands. If the sum overflows the range of
    the double type, the result in undefined.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fadd(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("double.sub", op1=cDouble, op2=cDouble, target=cDouble)
class Sub(Instruction):
    """
    Subtracts *op1* one from *op2*. If the difference underflows the range of
    the double type, the result in undefined.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fsub(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("double.mul", op1=cDouble, op2=cDouble, target=cDouble)
class Mul(Instruction):
    """
    Multiplies *op1* with *op2*. If the product overflows the range of the
    double type, the result in undefined.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fmul(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.constraint("int<32> | double")
def _int_or_double(ty, op, i):
    if isinstance(ty, type.Double):
        return (True, "")

    if isinstance(ty, type.Integer):
        return (True, "")

    return (False, "must be int<32> or double")

@hlt.instruction("double.pow", op1=cDouble, op2=_int_or_double, target=cDouble)
class Pow(Instruction):
    """Raises *op1* to the power *op2*. If the product overflows the range of
    the double type, the result in undefined.
    """
    def _validate(self, vld):
        if isinstance(self.op2().type(), type.Integer) and not self.op2().canCoerceTo(type.Integer(32)):
            vld.error(self.op2(), "cannot coerce operand 2 to int<32>")

    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())

        if isinstance(self.op2().type(), type.Integer):
            op2 = cg.llvmOp(self.op2(), coerce_to=type.Integer(32))
            rtype = cg.llvmType(self.target().type())
            result = cg.llvmCallIntrinsic(llvm.core.INTR_POWI, [rtype], [op1, op2])
            cg.llvmStoreInTarget(self, result)
        else:
            op2 = cg.llvmOp(self.op2())
            result = cg.llvmCallCInternal("pow", [op1, op2])
            cg.llvmStoreInTarget(self, result)

@hlt.instruction("double.div", op1=cDouble, op2=cNonZero(cDouble), target=cDouble)
class Div(Instruction):
    """
    Divides *op1* by *op2*, flooring the result.
    Throws :exc:`DivisionByZero` if *op2* is zero.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())

        block_ok = cg.llvmNewBlock("ok")
        block_exc = cg.llvmNewBlock("exc")

        iszero = cg.builder().fcmp(llvm.core.RPRED_ONE, op2, cg.llvmConstDouble(0))
        cg.builder().cbranch(iszero, block_ok, block_exc)

        cg.pushBuilder(block_exc)
        cg.llvmRaiseExceptionByName("hlt_exception_division_by_zero", self.location())
        cg.popBuilder()

        cg.pushBuilder(block_ok)
        result = cg.builder().fdiv(op1, op2)
        cg.llvmStoreInTarget(self, result)

        # Leave ok-builder for subsequent code.

@hlt.instruction("double.mod", op1=cDouble, op2=cNonZero(cDouble), target=cDouble)
class Mod(Instruction):
    """
    Calculates *op1* modulo *op2*.
    Throws :exc:`DivisionByZero` if *op2* is zero.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())

        block_ok = cg.llvmNewBlock("ok")
        block_exc = cg.llvmNewBlock("exc")

        iszero = cg.builder().fcmp(llvm.core.RPRED_ONE, op2, cg.llvmConstDouble(0))
        cg.builder().cbranch(iszero, block_ok, block_exc)

        cg.pushBuilder(block_exc)
        cg.llvmRaiseExceptionByName("hlt_exception_division_by_zero", self.location())
        cg.popBuilder()

        cg.pushBuilder(block_ok)
        result = cg.builder().frem(op1, op2)
        cg.llvmStoreInTarget(self, result)

        # Leave ok-builder for subsequent code.

@hlt.instruction("double.eq", op1=cDouble, op2=cDouble, target=cBool)
class Eq(Instruction):
    """
    Returns true iff *op1* equals *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fcmp(llvm.core.RPRED_OEQ, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("double.lt", op1=cDouble, op2=cDouble, target=cBool)
class Lt(Instruction):
    """
    Returns true iff *op1* is less than *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fcmp(llvm.core.RPRED_OLT, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("double.gt", op1=cDouble, op2=cDouble, target=cBool)
class Gt(Instruction):
    """
    Returns true iff *op1* is greater than *op2*.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fcmp(llvm.core.RPRED_OGT, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("double.as_uint", op1=cDouble, target=cInteger)
class AsUint(Instruction):
    """Converts the double *op1* into an unsigned integer value, rounding to
    the nearest value.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        result = cg.builder().fptoui(op1, cg.llvmType(self.target().type()))
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("double.as_sint", op1=cDouble, target=cInteger)
class AsUint(Instruction):
    """Converts the double *op1* into a signed integer value, rounding to
    the nearest value.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        result = cg.builder().fptosi(op1, cg.llvmType(self.target().type()))
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("double.as_time", op1=cDouble, target=cTime)
class AsTime(Instruction):
    """Converts the double *op1* into a time value, interpreting it as seconds
    since the epoch.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        val = cg.builder().fmul(op1, llvm.core.Constant.real(llvm.core.Type.double(), "1e9"))
        val = cg.builder().fptoui(val, llvm.core.Type.int(64))
        cg.llvmStoreInTarget(self, val)

@hlt.instruction("double.as_interval", op1=cDouble, target=cInterval)
class AsInterval(Instruction):
    """Converts the double *op1* into an interval value, interpreting it as
    seconds.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        val = cg.builder().fmul(op1, llvm.core.Constant.real(llvm.core.Type.double(), "1e9"))
        val = cg.builder().fptoui(val, llvm.core.Type.int(64))
        cg.llvmStoreInTarget(self, val)

