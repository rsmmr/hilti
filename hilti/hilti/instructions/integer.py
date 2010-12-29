# $Id$
"""
.. hlt:type:: int

   The *integer* data type represents signed integers of a fixed width. The
   width is specified as part of the type name as, e.g., in ``int<16>`` for a
   16-bit integer. There are predefined shortcuts ``int8``, ``int16``,
   ``int32`` and ``int64``. If not explictly initialized, integers are set to
   zero initially.

   TODO: Discuss signed vs. unsigned (short version: we don't distinguish, but
   have to version of all instructions for which it matters).
"""

import llvm.core

import hilti.system as system
import hilti.type as type

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("int", 1, c="int<n>_t")
class Integer(type.ValueType, type.Constable, type.Unpackable, type.Parameterizable):
    """Type for integers.

    width: The bit width.

    location: ~~Location - Location information for the type.
    """
    def __init__(self, n, location=None):
        super(Integer, self).__init__(location=location)
        self._width = n

    def width(self):
        """Returns the bit-width of the type's integers.

        Returns: int - The number of bits available to represent integers of
        this type.
        """
        return self._width

    @staticmethod
    def llvmUnpackType(fmt):
        """Static method that returns the type that ~~llvmUnpack will return
        for constant unpack formats.

        fmt: string - The fully qualified name of the format (e..g,
        ``Hilti::Packed::Int16Little``).

        Return: type.Type - The type.

        Note: Note this only applies to constant format. See ~~formats.
        """

        key = fmt.split("::")[-1]
        key = _makeIdx(key, False)

        try:
            return type.Integer(_Unpacks[key[0]][2])
        except KeyError:
            util.error("unknown format %s in Integer.llvmUnpackType")

    ### Overridden from Type.

    def name(self):
        return "int<%d>" % self._width

    def canCoerceTo(self, dsttype):
        if not isinstance(dsttype, type.Integer):
            return False

        if dsttype.width() == 0 or dsttype.width() == self._width:
            return True

        return False

    def cmpWithSameType(self, other):
        if self._width == 0 or other._width == 0:
            return True

        return self._width == other._width

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        """An ``int<n>`` is mapped to C integers depending on its width *n*,
        per the following table:

        ======  =======
        Width   C type
        ------  -------
        1..8    int8_t
        9..16   int16_t
        17..32  int32_t
        33..64  int64_t
        ======  =======
        """
        return llvm.core.Type.int(self.width())

    def _validate(self, vld):
        super(Integer, self)._validate(vld)
        if self._width == 0:
            vld.error(self, "integer type cannot have zero width ")

        if self._width > 64:
            vld.error(self, "maximum width for integer types is 64 bits")

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::int_to_string";
        typeinfo.to_int64 = "hlt::int_to_int64";

        for (w, p) in [(8, "int8_t"), (16, "int16_t"), (32, "int32_t"), (64, "int64_t")]:
            if self.width() <= w:
                typeinfo.c_prototype = p
                break
        else:
            assert False

        return typeinfo

    def llvmDefault(self, cg):
        return cg.llvmConstInt(0, self._width)

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        assert isinstance(const.value(), int)
        if not self._inRange(const.value()):
            vld.error(self, "integer constant out of type's range")

    def canCoerceConstantTo(self, value, dsttype):
        if not isinstance(dsttype, type.Integer):
            return False

        return dsttype._inRange(value.value())

    def coerceConstantTo(self, cg, value, dsttype):
        assert self.canCoerceConstantTo(value, dsttype)
        return constant.Constant(value.value(), dsttype, location=value.location)

    def llvmConstant(self, cg, const):
        return cg.llvmConstInt(const.value(), self._width)

    def outputConstant(self, printer, const):
        printer.output("%d" % const.value())

    ### Overridden from Parameterizable.

    def args(self):
        return [self._width]

    ### Overridden from Unpackable.

    def formats(self, mod):
        """Integer unpacking behaves slightly different depending whether the
        ``Hilti::Packed`` format is given as a constant or not. If it's not a
        constant, the unpacked integer will always have a width of 64 bits,
        independent of what kind of integer is stored in the binary data. If
        it is a constant, then for the signed variants (self.e.,
        ``Hilti::Packed::Int*``), the width of the target integer corresponds
        to the width of the unpacked integer. For the unsigned variants
        (self.e.,``Hilti::Packet::UInt*``), the width of the target integer
        must be "one step" *larger* than that of the unpacked value: for
        ``Packed::Uint8`` is must be ``int16`, for ``Packed::UInt8`` is must
        be ``int32`, etc. This is because we don't have *unsigned* integers in
        HILTself.

        Optionally, an additional arguments may be given, which then must be a
        ``tuple<int<8>,int<8>``. If given, the tuple specifies a bit range too
        extract from the unpacked value. For example, ``(6,9)`` extracts the bits
        6-9. The extraced bits are shifted to the right so that they align at bit
        0 and the result is returned (with the same width as it would have had
        without extracting any subset of bits). Note that the order in which bits
        are counted is determined by the endianess (e.g., in big-endian, we count
        from left to right).
        """

        tup = type.Tuple([type.Integer(8)] *2)
        return [(_makeIdx(key, False), tup, True, "Todo") for key in _Unpacks]

    def llvmUnpack(self, cg, begin, end, fmt, arg):
        val = cg.llvmAlloca(cg.llvmType(self))
        iter = cg.llvmAlloca(cg.llvmType(type.IteratorBytes()))

        # Generate the unpack code for a single case.
        def unpackOne(spec):
            def _unpackOne(case):
                # FIXME: We don't check the end position yet.
                (id, width, signed, little, bytes) = spec

                builder = cg.builder()
                itype = llvm.core.Type.int(width)
                result = llvm.core.Constant.null(itype)
                adjust_bits = 0

                # Copy the iterator.
                builder.store(begin, iter)

                # Function is defined in hilti_intern.ll
                exception = cg.llvmFrameExceptionAddr()
                ctx = cg.llvmCurrentExecutionContextPtr()

                for i in range(len(bytes)):
                    byte = cg.llvmCallCInternal("__hlt_bytes_extract_one", [iter, end, exception, ctx])
                    cg.llvmExceptionTest(exception)

                    builder = cg.builder()
                    byte.calling_convention = llvm.core.CC_C
                    byte = builder.zext(byte, itype)
                    if bytes[i]:
                        byte = builder.shl(byte, cg.llvmConstInt(bytes[i] * 8, width))
                    result = builder.or_(result, byte)

                if self._width > width:
                    if signed:
                        result = builder.sext(result, llvm.core.Type.int(self._width))
                    else:
                        result = builder.zext(result, llvm.core.Type.int(self._width))

                if arg:
                    builder = cg.builder()

                    # Extract bits.
                    llarg = arg.llvmLoad(cg)
                    low = cg.llvmExtractValue(llarg, 0)
                    high = cg.llvmExtractValue(llarg, 1)

                    def normalize(i):
                        # FIXME: Ideally, these int should come in here in a
                        # well-defined width. Currently, they don't ...
                        if i.type.width < width:
                            if signed:
                                i = cg.builder().sext(i, llvm.core.Type.int(width))
                            else:
                                i = cg.builder().zext(i, llvm.core.Type.int(width))

                        if i.type.width > width:
                            i = cg.builder().trunc(i, llvm.core.Type.int(width))

                        return i

                    low = normalize(low)
                    high = normalize(high)

                    if not little:
                        # Switch order of bits, in big-endian we're counting from
                        # the left.
                        low = cg.builder().add(cg.llvmConstInt(adjust_bits, width), low)
                        low = cg.builder().sub(cg.llvmConstInt(width - 1, width), low)
                        high = cg.builder().add(cg.llvmConstInt(adjust_bits, width), high)
                        high = cg.builder().sub(cg.llvmConstInt(width - 1, width), high)
                        tmp = high
                        high = low
                        low = tmp

                    result = _extractBits(cg, result, low, high)

                cg.llvmAssign(result, val)

            return _unpackOne

        cases = [(_makeIdx(key, True), unpackOne(spec)) for (key, spec) in _Unpacks.items()]
        cg.llvmSwitch(fmt, cases)
        return (cg.builder().load(val), cg.builder().load(iter))

    ### Private

    def _inRange(self, n):
        assert isinstance(n, int)
        return n < (2 ** self._width)

@hlt.constraint("int<*>")
def _unpackSize(ty, op, i):
    # Constraint function that enforces the correct size on the target.
    #
    # FIXME: Not implemented.
    return (True, "")

@hlt.overload(Incr, op1=cIntegerOfWidthAsOp(0), target=cInteger)
class Incr(Operator):
    """
    Returns the result of ``op1 + 1``. The result is undefined if an
    overflow occurs.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1(), self.target().type())
        op2 = cg.llvmConstInt(1, self.target().type().width())
        result = cg.builder().add(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Decr, op1=cIntegerOfWidthAsOp(0), target=cInteger)
class Decr(Operator):
    """
    Returns the result of ``op1 - 1``. The result is undefined if an
    overflow occurs.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1(), self.target().type())
        op2 = cg.llvmConstInt(1, self.target().type().width())
        result = cg.builder().sub(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Equal, op1=cInteger, op2=cIntegerOfWidthAsOp(1), target=cBool)
class Equal(Operator):
    """
    Returns True if *op1* equals *op2*.
    """
    def _codegen(self, cg):
        t = operand.coerceTypes(self.op1(), self.op2())
        assert t
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)

        result = cg.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.add", op1=cIntegerOfWidthAsOp(0), op2=cIntegerOfWidthAsOp(0), target=cInteger)
class Add(Instruction):
    """
    Calculates the sum of the two operands. Operands and target must be of
    same width. The result is calculated modulo 2^{width}.
    """
    def _codegen(self, cg):
        t = self.target().type()
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().add(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.sub", op1=cIntegerOfWidthAsOp(0), op2=cIntegerOfWidthAsOp(0), target=cInteger)
class Sub(Instruction):
    """
    Subtracts *op2* from *op1*. Operands and target must be of same width.
    The result is calculated modulo 2^{width}.
    """
    def _codegen(self, cg):
        t = self.target().type()
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().sub(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.mul", op1=cIntegerOfWidthAsOp(0), op2=cIntegerOfWidthAsOp(0), target=cInteger)
class Mul(Instruction):
    """
    Multiplies *op1* with *op2*. Operands and target must be of same width.
    The result is calculated modulo 2^{width}.
    """
    def _codegen(self, cg):
        t = self.target().type()
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().mul(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.div", op1=cIntegerOfWidthAsOp(0), op2=cNonZero(cIntegerOfWidthAsOp(0)), target=cInteger)
class Div(Instruction):
    """
    Divides *op1* by *op2*, flooring the result. Operands and target must be
    of same width.  If the product overflows the range of the integer type,
    the result in undefined.

    Throws :exc:`DivisionByZero` if *op2* is zero.
    """
    def _codegen(self, cg):
        t = self.target().type()
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)

        block_ok = cg.llvmNewBlock("ok")
        block_exc = cg.llvmNewBlock("exc")

        iszero = cg.builder().icmp(llvm.core.IPRED_NE, op2, cg.llvmConstInt(0, op2.type.width))
        cg.builder().cbranch(iszero, block_ok, block_exc)

        cg.pushBuilder(block_exc)
        cg.llvmRaiseExceptionByName("hlt_exception_division_by_zero", self.location())
        cg.popBuilder()

        cg.pushBuilder(block_ok)
        result = cg.builder().sdiv(op1, op2)
        cg.llvmStoreInTarget(self, result)

        # Leave ok-builder for subsequent code.

@hlt.instruction("int.mod", op1=cIntegerOfWidthAsOp(0), op2=cNonZero(cIntegerOfWidthAsOp(0)), target=cInteger)
class Mod(Instruction):
    """
    Calculates the remainder of dividing *op1* by *op2*. Operands and target must
    be of same width.

    Throws :exc:`DivisionByZero` if *op2* is zero.
    """
    def _codegen(self, cg):
        t = self.target().type()
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)

        block_ok = cg.llvmNewBlock("ok")
        block_exc = cg.llvmNewBlock("exc")

        iszero = cg.builder().icmp(llvm.core.IPRED_NE, op2, cg.llvmConstInt(0, self.op2.type.width))
        cg.builder().cbranch(iszero, block_ok, block_exc)

        cg.pushBuilder(block_exc)
        cg.llvmRaiseExceptionByName("hlt_exception_division_by_zero", self.location())
        cg.popBuilder()

        cg.pushBuilder(block_ok)
        result = cg.builder().srem(op1, op2)
        cg.llvmStoreInTarget(self, result)

        # Leave ok-builder for subsequent code.

@hlt.instruction("int.eq", op1=cInteger, op2=cIntegerOfWidthAsOp(1), target=cBool)
class Eq(Instruction):
    """
    Returns true iff *op1* equals *op2*.
    """
    def _codegen(self, cg):
        t = operand.coerceTypes(self.op1(), self.op2())
        assert t
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.sleq", op1=cInteger, op2=cIntegerOfWidthAsOp(1), target=cBool)
class Sleq(Instruction):
    """
    Returns true iff *op1* is lower or equal *op2*, interpreting both as
    *signed* integers.
    """
    def _codegen(self, cg):
        t = operand.coerceTypes(self.op1(), self.op2())
        assert t
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().icmp(llvm.core.IPRED_SLE, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.uleq", op1=cInteger, op2=cIntegerOfWidthAsOp(1), target=cBool)
class Sleq(Instruction):
    """
    Returns true iff *op1* is lower or equal *op2*, interpreting both as
    *unsigned* integers.
    """
    def _codegen(self, cg):
        t = operand.coerceTypes(self.op1(), self.op2())
        assert t
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().icmp(llvm.core.IPRED_ULE, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.sgeq", op1=cInteger, op2=cIntegerOfWidthAsOp(1), target=cBool)
class Sqeq(Instruction):
    """
    Returns true iff *op1* is greater or equal *op2*, interpreting both as
    *signed* integers.
    """
    def _codegen(self, cg):
        t = operand.coerceTypes(self.op1(), self.op2())
        assert t
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().icmp(llvm.core.IPRED_SGE, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.ugeq", op1=cInteger, op2=cIntegerOfWidthAsOp(1), target=cBool)
class Sqeq(Instruction):
    """
    Returns true iff *op1* is greater or equal *op2*, interpreting both as
    *unsigned* integers.
    """
    def _codegen(self, cg):
        t = operand.coerceTypes(self.op1(), self.op2())
        assert t
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().icmp(llvm.core.IPRED_UGE, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.slt", op1=cInteger, op2=cIntegerOfWidthAsOp(1), target=cBool)
class Slt(Instruction):
    """
    Returns true iff *op1* is less than *op2*, interpreting both as *signed*
    integers.
    """
    def _codegen(self, cg):
        t = operand.coerceTypes(self.op1(), self.op2())
        assert t
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().icmp(llvm.core.IPRED_SLT, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.ult", op1=cInteger, op2=cIntegerOfWidthAsOp(1), target=cBool)
class Ult(Instruction):
    """
    Returns true iff *op1* is less than *op2*, interpreting both as *unsigned*
    integers.
    """
    def _codegen(self, cg):
        t = operand.coerceTypes(self.op1(), self.op2())
        assert t
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().icmp(llvm.core.IPRED_ULT, op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.zext", op1=cInteger, target=cInteger)
class ZExt(Instruction):
    """
    Zero-extends *op1* into an integer of the same width as the *target*. The
    width of *op1* must be smaller or equal that of the *target*.
    """
    def _validate(self, vld):
        super(ZExt, self)._validate(vld)

        if self.op1().type().width() > self.target().type().width():
            vld.error(self, "width of integer operand too large")

    def _codegen(self, cg):
        width = self.target().type().width()
        assert width >= self.op1().type().width()

        op1 = cg.llvmOp(self.op1())

        result = cg.builder().zext(op1, cg.llvmType(type.Integer(width)))
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.sext", op1=cInteger, target=cInteger)
class SExt(Instruction):
    """
    Sign-extends *op1* into an integer of the same width as the *target*. The
    width of *op1* must be smaller or equal that of the *target*.
    """
    def _validate(self, vld):
        super(SExt, self)._validate(vld)

        if self.op1().type().width() > self.target().type().width():
            vld.error(self, "width of integer operand too large")

    def _codegen(self, cg):
        width = self.target().type().width()
        assert width >= self.op1().type().width()

        op1 = cg.llvmOp(self.op1())

        result = cg.builder().sext(op1, cg.llvmType(type.Integer(width)))
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.trunc", op1=cInteger, target=cInteger)
class Trunc(Instruction):
    """
    Bit-truncates *op1* into an integer of the same width as the *target*. The
    width of *op1* must be larger or equal that of the *target*.
    """
    def _validate(self, vld):
        super(Trunc, self)._validate(vld)

        if self.op1().type().width() < self.target().type().width():
            vld.error(self, "width of integer operand too small")

    def _codegen(self, cg):
        width = self.target().type().width()
        assert width <= self.op1().type().width()

        op1 = cg.llvmOp(self.op1())

        result = cg.builder().trunc(op1, cg.llvmType(type.Integer(width)))
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.and", op1=cIntegerOfWidthAsOp(0), op2=cIntegerOfWidthAsOp(0), target=cInteger)
class And(Instruction):
    """Calculates the binary *and* of the two operands. Operands and target
    must be of same width.
    """
    def _codegen(self, cg):
        t = self.target().type()
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().and_(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.or", op1=cIntegerOfWidthAsOp(0), op2=cIntegerOfWidthAsOp(0), target=cInteger)
class Or(Instruction):
    """Calculates the binary *or* of the two operands. Operands and target
    must be of same width.
    """
    def _codegen(self, cg):
        t = self.target().type()
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().or_(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.xor", op1=cIntegerOfWidthAsOp(0), op2=cIntegerOfWidthAsOp(0), target=cInteger)
class Xor(Instruction):
    """Calculates the binary *xor* of the two operands. Operands and target
    must be of same width.
    """
    def _codegen(self, cg):
        t = self.target().type()
        op1 = cg.llvmOp(self.op1(), t)
        op2 = cg.llvmOp(self.op2(), t)
        result = cg.builder().xor(op1, op2)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("int.mask", op1=cIntegerOfWidthAsOp(0), op2=cConstant(cIntegerOfWidth(64)), op3=cConstant(cIntegerOfWidth(64)), target=cInteger)
class Xor(Instruction):
    """Extracts the bits *op2* to *op3* (inclusive) from *op1* and shifts them
    so that they align with the least significant bit in the result.
    """
    def _validate(self, vld):
        lower = self.op2().value().value()
        upper = self.op3().value().value()

        if lower < 0 or upper > self.op1().type().width() or upper < lower:
            vld.error("invalid bit range")

    def _codegen(self, cg):
        t = self.target().type()
        width = t.width()
        op1 = cg.llvmOp(self.op1(), t)

        low = cg.llvmConstInt(self.op2().value().value(), width)
        high = cg.llvmConstInt(self.op3().value().value(), width)

        result = _extractBits(cg, op1, low, high)
        cg.llvmStoreInTarget(self, result)

# tag, id, width, signed, little, bytes
_Unpacks = {
    "Int8Little": (0,  8, True, True, [0]),
    "Int16Little": (1, 16, True, True, [0, 1]),
    "Int32Little": (2, 32, True, True, [0, 1, 2, 3]),
    "Int64Little": (3, 64, True, True, [0, 1, 2, 3, 4, 5, 6, 7]),

    "Int8Big":  (4,  8, True, False, [0]),
    "Int16Big": (5, 16, True, False, [1, 0]),
    "Int32Big": (6, 32, True, False, [3, 2, 1, 0]),
    "Int64Big": (7, 64, True, False, [7, 6, 5, 4, 3, 2, 1, 0]),

    "UInt8Little":  (8,  8, False, True, [0]),
    "UInt16Little": (9, 16, False, True, [0, 1]),
    "UInt32Little": (10, 32, False, True, [0, 1, 2, 3]),
    "UInt64Little": (11, 64, False, True, [0, 1, 2, 3, 4, 5, 6, 7]),

    "UInt8Big":  (12,  8, False, False, [0]),
    "UInt16Big": (13, 16, False, False, [1, 0]),
    "UInt32Big": (14, 32, False, False, [3, 2, 1, 0]),
    "UInt64Big": (15, 64, False, False, [7, 6, 5, 4, 3, 2, 1]),
}

_localSuffix = "Little" if system.isLittleEndian() else "Big"

def _makeIdx(key, scope):

    if scope:
        key = "Hilti::Packed::%s" % key

    if not key.endswith(_localSuffix):
        return key

    return [key, key[0:-len(_localSuffix)]]

def _extractBits(cg, result, low, high):

    # In theory, LLVM has an intrinsic for this. It practice it doesn't:
    #
    # Assertion failed: (0 && "part_select intrinsic not
    # implemented"), function visitIntrinsicCall, file
    # SelectionDAGBuild.cpp, line 3787.
    #
    # return cg.llvmCallIntrinsic(llvm.core.INTR_PART_SELECT, [result.type], [result, low, high])

    builder = cg.builder()
    width = result.type.width
    result = builder.lshr(result, low)
    bits = builder.sub(cg.llvmConstInt(width, width), high)
    bits = builder.add(bits, low)
    bits = builder.sub(bits, cg.llvmConstInt(1, width))
    mask = builder.lshr(cg.llvmConstInt(-1, width), bits)
    return  builder.and_(result, mask)
