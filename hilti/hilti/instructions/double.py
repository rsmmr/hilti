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
        """A ``double`` is intially initialized to zero."""
        return cg.llvmConstDouble(0)

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        return isinstance(const.value(), float)

    def llvmConstant(self, cg, const):
        return cg.llvmConstDouble(const.value())

    def outputConstant(self, printer, const):
        printer.output("%f" % const.value())

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
