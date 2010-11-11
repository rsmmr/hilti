# $Id$
#
# The signed integer type.

import integer

import binpac.type as type
import binpac.expr as expr
import binpac.operator as operator

import hilti.type

@type.pac("int")
class SignedInteger(type.Integer):
    """Type for signed integers.

    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(SignedInteger, self).__init__(width, location=location)


    ### Overridden from Type.

    def name(self):
        return "int%d" % self.width()

    def hiltiType(self, cg):
        return hilti.type.Integer(self.width())

    def hiltiCtor(self, cg, ctor):
        const = hilti.constant.Constant(ctor, hilti.type.Integer(self.width()))
        return hilti.operand.Constant(const)

    ### Overridden from ParseableType.

    def production(self, field):
        # XXX
        pass

    def generateParser(self, cg, dst):
        pass

@operator.Coerce(type.SignedInteger)
class Coerce:
    def coerceCtorTo(ctor, dsttype):
        if isinstance(dsttype, type.Bool):
            return ctor != 0

        if isinstance(dsttype, type.UnsignedInteger) and ctor >= 0:
            return ctor

        raise operators.CoerceError

    def canCoerceExprTo(srctype, dsttype):
        if isinstance(dsttype, type.Integer):
            return True

        if isinstance(dsttype, type.Bool):
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
            cg.builder().int_sext(tmp, e.evaluate(cg))
            return expr.Hilti(tmp, dsttype)

        if dsttype.width() < e.type().width():
            tmp = cg.builder().addLocal("__truncated", dsttype.hiltiType(cg))
            cg.builder().int_trunc(tmp, e.evaluate(cg))
            return expr.Hilti(tmp, dsttype)

@operator.Equal(SignedInteger, type.UnsignedInteger)
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

@operator.LowerEqual(SignedInteger, type.UnsignedInteger)
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
        cg.builder().int_sleq(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.GreaterEqual(SignedInteger, type.UnsignedInteger)
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
        cg.builder().int_sgeq(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp



