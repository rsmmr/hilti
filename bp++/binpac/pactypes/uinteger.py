# $Id$
#
# The unsigned integer type.

import integer

import binpac.type as type
import binpac.expr as expr
import binpac.grammar as grammar
import binpac.util as util
import binpac.operator as operator

import hilti.type

@type.pac("uint")
class UnsignedInteger(type.Integer):
    """Type for unsigned integers.

    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(UnsignedInteger, self).__init__(width, location=location)
        self._bits = None

    def setBits(self, bits):
        """Sets names for indvidual bits of this integer, which can then be
        used to access subvalues.

        bits: dictionary string -> (int, int, attrs) - A dictionary mapping
        field names to the bit-range. (``(4,6, attrs)`` means bits 4 to
        (inclusive) 6; ``(1,1, attrs)`` means just bit 1). ``attrs`` is an
        optional list of (string, expr.Expr) pairs defining field attributes. 

        """
        import sys
        self._bits = bits

    def bits(self):
        """Returns the bit fields the integer is broken down into, if set.

        Returns: dictionary string -> (int, int, attrs), or None - A dictionary
        mapping field names to the bit-range. (``(4,6,attrs)`` means bits 4 to
        (inclusive) 6; ``(1,1,attrs)`` means just bit 1). ``attrs`` is an
        optional list of (string, expr.Expr) pairs defining field attributes. 
        """
        return self._bits

    ### Overridden from node.Node.

    def _validate(self, vld):
        super(UnsignedInteger, self).__init__(vld)
        for (lower, upper, attrs) in self._bits.values():
            if lower < 0 or upper >= self.width() or upper < lower:
                vld.error("invalid bit field specification (%d:%d)" % (lower, upper))

            for (attr, ex) in attrs:
                if attr != "convert":
                    vld.error("unsupport bitfield attribute %s" % attr)

                if not operator.typecheck(operator.Operator.Call, [ex, [expr.Hilti(None, UnsignedInteger(upper-lower+1))]]):
                    vld.error("no matching function for &convert found")

    ### Overridden from Type.

    def name(self):
        return "uint%d" % self.width()

    def hiltiType(self, cg):
        return hilti.type.Integer(self.width())

    def hiltiCtor(self, cg, ctor):
        const = hilti.constant.Constant(ctor, hilti.type.Integer(self.width()))
        return hilti.operand.Constant(const)

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
        util.internal_error("Type.productionForLiteral() not support for uint")

    def fieldType(self):
        filter = self.attributeExpr("convert")
        if filter:
            return filter.type()
        else:
            return self.parsedType()

    def parsedType(self):
        return self

    _packeds = {
        ("little", 8): "Hilti::Packed::UInt8Little",
        ("little", 16): "Hilti::Packed::UInt16Little",
        ("little", 32): "Hilti::Packed::UInt32Little",
        ("little", 64): "Hilti::Packed::Int64Little",
        ("big", 8): "Hilti::Packed::UInt8Big",
        ("big", 16): "Hilti::Packed::UInt16Big",
        ("big", 32): "Hilti::Packed::UInt32Big",
        ("big", 64): "Hilti::Packed::Int64Big",
        ("host", 8): "Hilti::Packed::UInt8",
        ("host", 16): "Hilti::Packed::UInt16",
        ("host", 32): "Hilti::Packed::UInt32",
        ("host", 64): "Hilti::Packed::Int64",
        ("network", 8): "Hilti::Packed::UInt8Big",
        ("network", 16): "Hilti::Packed::UInt16Big",
        ("network", 32): "Hilti::Packed::UInt32Big",
        ("network", 64): "Hilti::Packed::Int64Big",
        }

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
            op2 = self._hiltiByteOrderOp(cg, UnsignedInteger._packeds, self.width())
            op3 = None
            assert op2
        else:
            assert self.width() in (8, 16, 32, 64)
            op2 = cg.builder().idOp("Hilti::Packed::SkipBytesFixed")
            op3 = cg.builder().constOp(self.width() / 8, hilti.type.Integer(64))

        result = self.generateUnpack(cg, args, op1, op2, op3)

        builder = cg.builder()

        if dst and not skipping:
            builder.tuple_index(dst, result, builder.constOp(0))

        builder.tuple_index(cur, result, builder.constOp(1))

        return cur


@operator.Coerce(type.UnsignedInteger)
class Coerce:
    def coerceCtorTo(ctor, dsttype):
        if isinstance(dsttype, type.Bool):
            return ctor != 0

        if isinstance(dsttype, type.Integer):
            return ctor

        raise operator.CoerceError

    def canCoerceTo(srctype, dsttype):
        if isinstance(dsttype, type.Bool):
            return True

        if isinstance(dsttype, type.UnsignedInteger):
            return True

        return False

    def coerceExprTo(cg, e, dsttype):
        if isinstance(dsttype, type.Bool):
            tmp = cg.builder().addLocal("__nonempty", hilti.type.Bool())
            cg.builder().equal(tmp, e.evaluate(cg), cg.builder().constOp(0))
            cg.builder().bool_not(tmp, tmp)
            return expr.Hilti(tmp, type.Bool())

        if dsttype.width() == e.type().width():
            return e

        if dsttype.width() > e.type().width():
            tmp = cg.builder().addLocal("__extended", dsttype.hiltiType(cg))
            cg.builder().int_zext(tmp, e.evaluate(cg))
            return expr.Hilti(tmp, dsttype)

        if dsttype.width() < e.type().width():
            tmp = cg.builder().addLocal("__truncated", dsttype.hiltiType(cg))
            cg.builder().int_trunc(tmp, e.evaluate(cg))
            return expr.Hilti(tmp, dsttype)

def _dstType(expr1, expr2):
    return UnsignedInteger(max(expr1.type().width(), expr2.type().width()))

def _extendOps(cg, expr1, expr2):
    w = max(expr1.type().width(), expr2.type().width())
    expr1 = expr1.evaluate(cg)
    expr2 = expr2.evaluate(cg)
    tmp = None

    if expr1.type().width() < w:
        tmp = cg.builder().addLocal("__extop1", hilti.type.Integer(w))
        cg.builder().int_zext(tmp, expr1)
        expr1 = tmp

    if expr2.type().width() < w:
        tmp = cg.builder().addLocal("__extop2", hilti.type.Integer(w))
        cg.builder().int_zext(tmp, expr2)
        expr2 = tmp

    return (expr1, expr2)

@operator.Plus(UnsignedInteger, UnsignedInteger)
class Plus:

    def type(expr1, expr2):
        return _dstType(expr1, expr2)

    def simplify(expr1, expr2):
        if isinstance(expr1, expr.Ctor) and isinstance(expr2, expr.Ctor):
            val = expr1.value() + expr2.value()
            return expr.Ctor(val, _dstType(expr1, expr2))

        else:
            return None

    def evaluate(cg, expr1, expr2):
        (expr1, expr2) = _extendOps(cg, expr1, expr2)
        tmp = cg.builder().addLocal("__sum", hilti.type.Integer(expr1.type().width()))
        cg.builder().int_add(tmp, expr1, expr2)
        return tmp

@operator.Minus(UnsignedInteger, UnsignedInteger)
class Minus:

    def type(expr1, expr2):
        return _dstType(expr1, expr2)

    def simplify(expr1, expr2):
        if isinstance(expr1, expr.Ctor) and isinstance(expr2, expr.Ctor):
            val = expr1.value() - expr2.value()
            return expr.Ctor(val, _dstType(expr1, expr2))

        else:
            return None

    def evaluate(cg, expr1, expr2):
        (expr1, expr2) = _extendOps(cg, expr1, expr2)
        tmp = cg.builder().addLocal("__sum", hilti.type.Integer(expr1.type().width()))
        cg.builder().int_sub(tmp, expr1, expr2)
        return tmp

@operator.Equal(UnsignedInteger, type.UnsignedInteger)
class _:
    def type(e1, e2):
        return type.Bool()

    def simplify(e1, e2):
        if not e1.isInit() or not e2.isInit():
            return None

        b = (e1.value() == e2.value())
        return expr.Ctor(b, type.Bool())

    def evaluate(cg, e1, e2):
        tmp = cg.functionBuilder().addLocal("__equal", hilti.type.Bool())
        cg.builder().equal(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.LowerEqual(UnsignedInteger, type.UnsignedInteger)
class _:
    def type(e1, e2):
        return type.Bool()

    def simplify(e1, e2):
        if not e1.isInit() or not e2.isInit():
            return None

        b = (e1.value() <= e2.value())
        return expr.Ctor(b, type.Bool())

    def evaluate(cg, e1, e2):
        tmp = cg.functionBuilder().addLocal("__leq", hilti.type.Bool())
        cg.builder().int_uleq(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.GreaterEqual(UnsignedInteger, type.UnsignedInteger)
class _:
    def type(e1, e2):
        return type.Bool()

    def simplify(e1, e2):
        if not e1.isInit() or not e2.isInit():
            return None

        b = (e1.value() >= e2.value())
        return expr.Ctor(b, type.Bool())

    def evaluate(cg, e1, e2):
        tmp = cg.functionBuilder().addLocal("__geq", hilti.type.Bool())
        cg.builder().int_ugeq(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Attribute(UnsignedInteger, type.String)
class Attribute:
    def validate(vld, lhs, ident):
        name = ident.name()
        if not lhs.type().bits():
            vld.error(lhs, "unknown attribute '%s'" % name)

        if name and not name in lhs.type().bits():
            vld.error(lhs, "unknown bitset field '%s'" % name)

    def type(lhs, ident):
        return lhs.type()

    def evaluate(cg, lhs, ident):
        name = ident.name()
        builder = cg.builder()
        (lower, upper, attrs) = lhs.type().bits()[name]
        tmp = builder.addLocal("__bits", lhs.type().hiltiType(cg))
        cg.builder().int_mask(tmp, lhs.evaluate(cg), builder.constOp(lower), builder.constOp(upper))

        for (attr, ex) in attrs:
            assert attr == "convert"
            tmp = operator.evaluate(operator.Operator.Call, cg, [ex, [expr.Hilti(tmp, UnsignedInteger(tmp.type().width()))]])

        return tmp

    def assign(cg, lhs, ident, rhs):
        util.internal_error("assigning to bitfields is not support currently")

