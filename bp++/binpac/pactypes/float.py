# $Id$
#
# The float type.

import binpac.type as type
import binpac.expr as expr
import binpac.operator as operator

import hilti.type

@type.pac("float")
class Float(type.ParseableType):
    """Type for float objects.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Float, self).__init__(location=location)

    ### Overridden from Type.

    def validate(self, vld):
        return True

    def validateCtor(self, vld, ctor):
        if not isinstance(ctor, float) and not isinstance(ctor, tuple):
            vld.error(ctor, "float: constant of wrong internal type")

    def hiltiType(self, cg):
        return hilti.type.Double()

    def hiltiCtor(self, cg, ctor):
        const = hilti.constant.Constant(ctor, hilti.type.Double())
        return hilti.operand.Constant(const)

    def name(self):
        return "float"

    def pac(self, printer):
        printer.output("float")

    def pacCtor(self, printer, value):
        printer.output("%f" % value)

    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return {}

    def production(self, field):
        util.internal_error("float parsing not implemented")

    def generateParser(self, cg, var, args, dst, skipping):
        util.internal_error("float parsing not implemented")

@operator.Plus(Float, Float)
class _:
    def type(e1, e2):
        return Float()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Double())
        cg.builder().double_add(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Minus(Float, Float)
class _:
    def type(e1, e2):
        return Float()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Double())
        cg.builder().double_sub(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Div(Float, Float)
class _:
    def type(e1, e2):
        return Float()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Double())
        cg.builder().double_div(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Mod(Float, Float)
class _:
    def type(e1, e2):
        return Float()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Double())
        cg.builder().double_mod(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Mult(Float, Float)
class _:
    def type(e1, e2):
        return Float()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Double())
        cg.builder().double_mul(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Power(Float, Float)
class _:
    def type(e1, e2):
        return Float()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Double())
        cg.builder().double_pow(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Equal(Float, Float)
class _:
    def type(e1, e2):
        return type.Bool()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Bool())
        cg.builder().double_eq(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Lower(Float, Float)
class _:
    def type(e1, e2):
        return type.Bool()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Bool())
        cg.builder().double_lt(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Greater(Float, Float)
class _:
    def type(e1, e2):
        return type.Bool()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Bool())
        cg.builder().double_gt(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Coerce(Float, type.Bool)
class _:
    def coerceCtorTo(ctor, dsttype):
        return ctor != 0

    def coerceTo(cg, e, dsttype):
        tmp = cg.builder().addLocal("__nonempty", hilti.type.Bool())
        cg.builder().equal(tmp, e.evaluate(cg), 0.0)
        cg.builder().bool_not(tmp, tmp)
        return expr.Hilti(tmp, type.Bool())

# We do that here to avoid recursive dependencies in integer.py.

@operator.Coerce(type.SignedInteger, Float)
class _:
    def coerceCtorTo(ctor, dsttype):
        return (ctor, 0)

    def coerceTo(cg, e, dsttype):
        tmp = cg.builder().addLocal("__float", hilti.type.Double())
        cg.builder().int_sdouble(tmp, e.evaluate(cg))
        return tmp

    def typeCoerceTo(e, dsttype):
        return hilti.type.Double()

@operator.Coerce(type.UnsignedInteger, Float)
class _:
    def coerceCtorTo(ctor, dsttype):
        return (ctor, 0)

    def coerceTo(cg, e, dsttype):
        tmp = cg.builder().addLocal("__float", hilti.type.Double())
        cg.builder().int_udouble(tmp, e.evaluate(cg))
        return tmp

    def typeCoerceTo(e, dsttype):
        return hilti.type.Double()

@operator.Cast(Float, type.SignedInteger)
class _:
    def type(e, t):
        return t

    def evaluate(cg, e, t):
        tmp = cg.builder().addLocal("__sint", t.hiltiType(cg))
        cg.builder().double_sint(tmp, e.evaluate(cg))
        return tmp

@operator.Cast(Float, type.UnsignedInteger)
class _:
    def type(e, t):
        return t

    def evaluate(cg, e, t):
        tmp = cg.builder().addLocal("__uint", t.hiltiType(cg))
        cg.builder().double_uint(tmp, e.evaluate(cg))
        return tmp
