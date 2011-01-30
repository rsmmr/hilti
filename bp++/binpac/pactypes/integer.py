# $Id$
#
# Base class for integer types.
#
# TODO: Much of the functionality of SignedInteger and UnsignedInteger can be
# merged into the base class.

import binpac.type as type
import binpac.expr as expr
import binpac.grammar as grammar
import binpac.operator as operator

import hilti.type

@type.pac("int")
class Integer(type.ParseableWithByteOrder):
    """Base class for integers.

    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(Integer, self).__init__(location=location)
        assert(width > 0 and width <= 64)
        self._width = width
        self._bits = {}

    def width(self):
        """Returns the bit-width of the type's integers.

        Returns: int - The number of bits available to represent integers of
        this type.
        """
        return self._width

    def setWidth(self, width):
        """Sets the bit-width of the type's integers.

        width: int - The new bit-width.
        """
        self._width = width

    def setBits(self, bits):
        """Sets names for indvidual bits of this integer, which can then be
        used to access subvalues.

        bits: dictionary string -> (int, int, attrs) - A dictionary mapping
        field names to the bit-range. (``(4,6, attrs)`` means bits 4 to
        (inclusive) 6; ``(1,1, attrs)`` means just bit 1). ``attrs`` is an
        optional list of (string, expr.Expr) pairs defining field attributes.

        """
        self._bits = bits

    def bits(self):
        """Returns the bit fields the integer is broken down into, if set.

        Returns: dictionary string -> (int, int, attrs), or None - A dictionary
        mapping field names to the bit-range. (``(4,6,attrs)`` means bits 4 to
        (inclusive) 6; ``(1,1,attrs)`` means just bit 1). ``attrs`` is an
        optional list of (string, expr.Expr) pairs defining field attributes.
        """
        return self._bits

    ### Overridden from Type.

    def validate(self, vld):
        if self._width < 1 or self._width > 64:
            vld.error(self, "integer width out of range")

        for (lower, upper, attrs) in self._bits.values():
            if lower < 0 or upper >= self._usableBits() or upper < lower:
                vld.error("invalid bit field specification (%d:%d)" % (lower, upper))

            for (attr, ex) in attrs:
                if attr != "convert":
                    vld.error("unsupport bitfield attribute %s" % attr)

                if not operator.hasOperator(operator.Operator.Call, [ex, [expr.Hilti(None, self._typeOfWidth(upper-lower+1))]]):
                    vld.error("no matching function for &convert found")

    def validateCtor(self, vld, ctor):
        if not isinstance(ctor, int):
            vld.error(const, "integer: constant of wrong internal type")

        (min, max) = self._range()

        if ctor < min or ctor > max:
            vld.error(const, "integer value out of range")

    def hiltiType(self, cg):
        return hilti.type.Integer(self.width())

    def hiltiCtor(self, cg, ctor):
        const = hilti.constant.Constant(ctor, hilti.type.Integer(self.width()))
        return hilti.operand.Constant(const)

    def pac(self, printer):
        printer.output(self.name())

    def pacCtor(self, printer, ctor):
        printer.output("%d" % ctor)

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.width() == other.width()

        return NotImplemented

    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return {
            "default": (self, True, None),
            "byteorder": ("binpac::ByteOrder", False, None),
            "convert": (type.Any(), False, None),
            }

    def validateInUnit(self, field, vld):
        #if not self._getPackedEnum():
        #    vld.error("uint/byteorder combiniation not supported for parsing")
        pass

    def production(self, field):
        filter = self.attributeExpr("convert")
        return grammar.Variable(None, self, filter=filter, location=self.location())

    def productionForLiteral(self, field, literal):
        util.internal_error("Type.productionForLiteral() not support for integers")

    def fieldType(self):
        filter = self.attributeExpr("convert")
        if filter:
            return filter.type()
        else:
            return self.parsedType()

    def parsedType(self):
        return self

    def generateParser(self, cg, var, args, dst, skipping):
        cur = args.cur
        bytesit = hilti.type.IteratorBytes(hilti.type.Bytes())
        resultt = hilti.type.Tuple([self.hiltiType(cg), bytesit])
        fbuilder = cg.functionBuilder()

        # FIXME: We trust here that bytes iterators are inialized with the
        # generic end position. We should add a way to get that position
        # directly (but without a bytes object).
        end = fbuilder.addTmp("__end", bytesit)

        op1 = cg.builder().tupleOp([cur, end])

        if not skipping:
            op2 = self._hiltiByteOrderOp(cg, self._packEnums(), self.width())
            op3 = None
            assert op2
        else:
            assert self.width() in (8, 16, 32, 64)
            op2 = cg.builder().idOp("Hilti::Packed::SkipBytesFixed")
            op3 = cg.builder().constOp(self.width() / 8, hilti.type.Integer(64))

        result = self.generateUnpack(cg, args, op1, op2, op3)

        builder = cg.builder()

        if dst and not skipping:
            builder.tuple_index(dst, result, 0)

        builder.tuple_index(cur, result, 1)

        return cur

    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return {
            "default": (self, True, None),
            "byteorder": ("binpac::ByteOrder", False, None),
            "convert": (type.Any(), False, None),
            }

    ### For derived classed to override.

    def _instanceOfWidth(self, n):
        util.internal_error("method not overridden")

    def _usableBits(self):
        util.internal_error("method not overridden")

    def _packEnums(self):
        util.internal_error("method not overridden")

    def _range(self):
        util.internal_error("method not overridden")

    def _ext(self, cg, target, val):
        util.internal_error("method not overridden")

    def _leq(self, cg, target, val1, val2):
        util.internal_error("method not overridden")

    def _lt(self, cg, target, val1, val2):
        util.internal_error("method not overridden")

    def _geq(self, cg, target, val1, val2):
        util.internal_error("method not overridden")

    def _gt(self, cg, target, val1, val2):
        util.internal_error("method not overridden")

    def _shr(self, cg, target, val1, val2):
        util.internal_error("method not overridden")

def _dstType(e1, e2):
    return e1.type()._typeOfWidth(max(e1.type().width(), e2.type().width()))

def _extendOps(cg, e1, e2):
    ty = e1.type()
    w = max(e1.type().width(), e2.type().width())
    e1 = e1.evaluate(cg)
    e2 = e2.evaluate(cg)
    tmp = None

    if e1.type().width() < w:
        tmp = cg.builder().addLocal("__extop1", hilti.type.Integer(w))
        ty._ext(cg, tmp, e1)
        e1 = tmp

    if e2.type().width() < w:
        tmp = cg.builder().addLocal("__extop2", hilti.type.Integer(w))
        ty._ext(cg, tmp, e2)
        e2 = tmp

    return (e1, e2)


def _coerce_integer(ops):
    assert(len(ops)) == 2

    if isinstance(ops[0], expr.Ctor):
        val = ops[0].value()
        (min, max) = ops[1]._range()
        return operator.Match if val >= min and val <= max else operator.NoMatch

    for cls in (type.SignedInteger, type.UnsignedInteger):
        if isinstance(ops[0], cls) and isinstance(ops[1], cls):
            if ops[0].width() <= ops[1].width():
                return operator.NoMatch

    return operator.NoMatch

def _coerce_other_integer(ops):
    # The check for the first integer type already does what we need.
    return operator.Match

@operator.Plus(Integer, Integer)
class _:
    def type(e1, e2):
        return _dstType(e1, e2)

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def simplify(e1, e2):
        if isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor):
            val = e1.value() + e2.value()
            return expr.Ctor(val, _dstType(e1, e2))

        return None

    def evaluate(cg, e1, e2):
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.builder().addLocal("__sum", hilti.type.Integer(e1.type().width()))
        cg.builder().int_add(tmp, e1, e2)
        return tmp

@operator.Minus(Integer, Integer)
class _:
    def type(e1, e2):
        return _dstType(e1, e2)

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def simplify(e1, e2):
        if isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor):
            val = e1.value() - e2.value()
            return expr.Ctor(val, _dstType(e1, e2))

        return None

    def evaluate(cg, e1, e2):
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.builder().addLocal("__diff", hilti.type.Integer(e1.type().width()))
        cg.builder().int_sub(tmp, e1, e2)
        return tmp

@operator.Div(Integer, Integer)
class _:
    def type(e1, e2):
        return _dstType(e1, e2)

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def validate(vld, e1, e2):
        if isinstance(e2, expr.Ctor) and e2.value() == 0:
            vld.error(e2, "division by zero")

    def simplify(e1, e2):
        if isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor):
            val = e1.value() / e2.value()
            return expr.Ctor(val, _dstType(e1, e2))

        return None

    def evaluate(cg, e1, e2):
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.builder().addLocal("__div", hilti.type.Integer(e1.type().width()))
        cg.builder().int_div(tmp, e1, e2)
        return tmp

@operator.Mod(Integer, Integer)
class _:
    def type(e1, e2):
        return _dstType(e1, e2)

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def validate(vld, e1, e2):
        if isinstance(e2, expr.Ctor) and e2.value() == 0:
            vld.error(e2, "division by zero")

    def simplify(e1, e2):
        if isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor):
            val = e1.value() % e2.value()
            return expr.Ctor(val, _dstType(e1, e2))

        return None

    def evaluate(cg, e1, e2):
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.builder().addLocal("__rem", hilti.type.Integer(e1.type().width()))
        cg.builder().int_mod(tmp, e1, e2)
        return tmp

@operator.Mult(Integer, Integer)
class _:
    def type(e1, e2):
        return _dstType(e1, e2)

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def simplify(e1, e2):
        if isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor):
            val = e1.value() * e2.value()
            return expr.Ctor(val, _dstType(e1, e2))

        return None

    def evaluate(cg, e1, e2):
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.builder().addLocal("__mul", hilti.type.Integer(e1.type().width()))
        cg.builder().int_mul(tmp, e1, e2)
        return tmp

@operator.BitAnd(Integer, Integer)
class _:
    def type(e1, e2):
        return _dstType(e1, e2)

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def simplify(e1, e2):
        if isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor):
            val = e1.value() & e2.value()
            return expr.Ctor(val, _dstType(e1, e2))

        return None

    def evaluate(cg, e1, e2):
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.builder().addLocal("__and", hilti.type.Integer(e1.type().width()))
        cg.builder().int_and(tmp, e1, e2)
        return tmp

@operator.BitOr(Integer, Integer)
class _:
    def type(e1, e2):
        return _dstType(e1, e2)

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def simplify(e1, e2):
        if isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor):
            val = e1.value() | e2.value()
            return expr.Ctor(val, _dstType(e1, e2))

        return None

    def evaluate(cg, e1, e2):
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.builder().addLocal("__or", hilti.type.Integer(e1.type().width()))
        cg.builder().int_or(tmp, e1, e2)
        return tmp


@operator.BitXor(Integer, Integer)
class _:
    def type(e1, e2):
        return _dstType(e1, e2)

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def simplify(e1, e2):
        if isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor):
            val = e1.value() ^ e2.value()
            return expr.Ctor(val, _dstType(e1, e2))

        return None

    def evaluate(cg, e1, e2):
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.builder().addLocal("__xor", hilti.type.Integer(e1.type().width()))
        cg.builder().int_xor(tmp, e1, e2)
        return tmp

@operator.ShiftLeft(Integer, Integer)
class _:
    def type(e1, e2):
        return _dstType(e1, e2)

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def simplify(e1, e2):
        if isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor):
            val = e1.value() << e2.value()
            return expr.Ctor(val, _dstType(e1, e2))

        return None

    def evaluate(cg, e1, e2):
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.builder().addLocal("__shl", hilti.type.Integer(e1.type().width()))
        cg.builder().int_shl(tmp, e1, e2)
        return tmp

@operator.ShiftRight(Integer, Integer)
class _:
    def type(e1, e2):
        return _dstType(e1, e2)

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def simplify(e1, e2):
        if isinstance(e1, expr.Ctor) and isinstance(e2, expr.Ctor):
            val = e1.value() >> e2.value()
            return expr.Ctor(val, _dstType(e1, e2))

        return None

    def evaluate(cg, e1, e2):
        ty = e1.type()
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.builder().addLocal("__shr", hilti.type.Integer(e1.type().width()))
        ty._shr(cg, tmp, e1, e2)
        return tmp

@operator.Equal(Integer, Integer)
class _:
    def type(e1, e2):
        return type.Bool()

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def simplify(e1, e2):
        if not isinstance(e1, expr.Ctor) or not isinstance(e2, expr.Ctor):
            return None

        b = (e1.value() == e2.value())
        return expr.Ctor(b, type.Bool())

    def evaluate(cg, e1, e2):
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.functionBuilder().addLocal("__equal", hilti.type.Bool())
        cg.builder().equal(tmp, e1, e2)
        return tmp

@operator.Lower(Integer, Integer)
class _:
    def type(e1, e2):
        return type.Bool()

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def simplify(e1, e2):
        if not isinstance(e1, expr.Ctor) or not isinstance(e2, expr.Ctor):
            return None

        b = (e1.value() < e2.value())
        return expr.Ctor(b, type.Bool())

    def evaluate(cg, e1, e2):
        ty = e1.type()
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.functionBuilder().addLocal("__lt", hilti.type.Bool())
        ty._lt(cg, tmp, e1, e2)
        return tmp

@operator.Greater(Integer, Integer)
class _:
    def type(e1, e2):
        return type.Bool()

    def validate(vld, e1, e2):
        if e1.type().__class__ != e2.type().__class__:
            vld.error(e1, "mix of signed and unsigned integers")

    def simplify(e1, e2):
        if not isinstance(e1, expr.Ctor) or not isinstance(e2, expr.Ctor):
            return None

        b = (e1.value() > e2.value())
        return expr.Ctor(b, type.Bool())

    def evaluate(cg, e1, e2):
        ty = e1.type()
        (e1, e2) = _extendOps(cg, e1, e2)
        tmp = cg.functionBuilder().addLocal("__gt", hilti.type.Bool())
        ty._gt(cg, tmp, e1, e2)
        return tmp

@operator.Attribute(Integer, type.String)
class _:
    def type(lhs, ident):
        return lhs.type()

    def validate(vld, lhs, ident):
        name = ident.name()
        if not lhs.type().bits():
            vld.error(lhs, "unknown attribute '%s'" % name)

        if name and not name in lhs.type().bits():
            vld.error(lhs, "unknown bitset field '%s'" % name)

    def evaluate(cg, lhs, ident):
        name = ident.name()
        builder = cg.builder()
        (lower, upper, attrs) = lhs.type().bits()[name]
        tmp = builder.addLocal("__bits", lhs.type().hiltiType(cg))
        cg.builder().int_mask(tmp, lhs.evaluate(cg), builder.constOp(lower), upper)

        for (attr, ex) in attrs:
            assert attr == "convert"
            tmp = operator.evaluate(operator.Operator.Call, cg, [ex, [expr.Hilti(tmp, UnsignedInteger(tmp.type().width()))]])

        return tmp

    def assign(cg, lhs, ident, rhs):
        util.internal_error("assigning to bitfields is not support currently")

@operator.Coerce(Integer, type.Bool)
class _:
    def coerceCtorTo(ctor, dsttype):
        return ctor != 0

    def coerceTo(cg, e, dsttype):
        tmp = cg.builder().addLocal("__nonempty", hilti.type.Bool())
        cg.builder().equal(tmp, e.evaluate(cg), 0)
        cg.builder().bool_not(tmp, tmp)
        return expr.Hilti(tmp, type.Bool())

@operator.Coerce(_coerce_integer, _coerce_other_integer)
class _:
    def coerceCtorTo(ctor, dsttype):
        return ctor

    def coerceTo(cg, e, dsttype):
        if dsttype.width() == e.type().width():
            return e

        assert dsttype.width() > e.type().width()

        tmp = cg.builder().addLocal("__extended", dsttype.hiltiType(cg))
        dsttype._ext(cg, tmp, e.evaluate(cg))
        return expr.Hilti(tmp, dsttype)


@operator.Cast(Integer, Integer)
class _:
    def type(e, t):
        return t

    def evaluate(cg, e, t):
        et = e.type()

        if t.width() < et.width():
            # Truncate works for both signed and unsigned.
            tmp = cg.builder().addLocal("__truncated", t.hiltiType(cg))
            cg.builder().int_trunc(tmp, e.evaluate(cg))
            return tmp

        # The other cases for a cast to the same type have already been
        # handled through coercion, so if we arrive here, that means
        # we're casting signedness.

        if t.width() == et.width():
            # Nothing todo, just reinterpret.
            return e.evaluate(cg)

        if t.width() > et.width():
            # Need to extend. We reinterpret and extend according to target
            # type.
            tmp = cg.builder().addLocal("__extended", t.hiltiType(cg))
            t._ext(cg, tmp, e.evaluate(cg))
            return tmp

        # Can't get here.
        assert False
