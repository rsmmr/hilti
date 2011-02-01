# $Id$
#
# The interval type.

import binpac.type as type
import binpac.expr as expr
import binpac.operator as operator

import hilti.type

@type.pac("interval")
class Interval(type.Type):
    """Type for interval objects.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Interval, self).__init__(location=location)

    ### Overridden from Type.

    def validate(self, vld):
        return True

    def validateCtor(self, vld, ctor):
        if not (isinstance(ctor, tuple) and len(ctor) == 2) or \
           not isinstance(ctor[0], int) or \
           not isinstance(ctor[1], int):
            vld.error(ctor, "interval: constant of wrong internal type")

    def hiltiType(self, cg):
        return hilti.type.Interval()

    def hiltiCtor(self, cg, ctor):
        const = hilti.constant.Constant(ctor, hilti.type.Interval())
        return hilti.operand.Constant(const)

    def name(self):
        return "interval"

    def pac(self, printer):
        printer.output("interval")

    def pacCtor(self, printer, value):
        printer.output("%d.%09d", value[0], value[1])

@operator.Plus(Interval, Interval)
class _:
    def type(e1, e2):
        return Interval()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Interval())
        cg.builder().interval_add(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Minus(Interval, Interval)
class _:
    def type(e1, e2):
        return Interval()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Interval())
        cg.builder().interval_sub(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Mult(Interval, type.UnsignedInteger)
class _:
    def type(e1, e2):
        return Interval()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Interval())
        cg.builder().interval_mul(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Mult(type.UnsignedInteger, Interval)
class _:
    def type(e1, e2):
        return Interval()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Interval())
        cg.builder().interval_mul(tmp, e2.evaluate(cg), e1.evaluate(cg))
        return tmp

@operator.Equal(Interval, Interval)
class _:
    def type(e1, e2):
        return type.Bool()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Bool())
        cg.builder().interval_eq(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Lower(Interval, Interval)
class _:
    def type(e1, e2):
        return type.Bool()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Bool())
        cg.builder().interval_lt(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Greater(Interval, Interval)
class _:
    def type(e1, e2):
        return type.Bool()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Bool())
        cg.builder().interval_gt(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Coerce(type.Float, Interval)
class _:
    def coerceCtorTo(ctor, dsttype):
        return ctor

    def coerceTo(cg, e, dsttype):
        tmp = cg.builder().addLocal("__interval", hilti.type.Interval())
        cg.builder().float_as_interval(tmp, e.evaluate(cg))
        return tmp

@operator.Coerce(type.UnsignedInteger, Interval)
class _:
    def coerceCtorTo(ctor, dsttype):
        return (ctor, 0)

    def coerceTo(cg, e, dsttype):
        tmp = cg.builder().addLocal("__interval", hilti.type.Interval())
        cg.builder().int_as_interval(tmp, e.evaluate(cg))
        return tmp

@operator.Cast(Interval, type.UnsignedInteger)
class _:
    def type(e, t):
        return t

    def evaluate(cg, e, t):
        tmp = cg.builder().addLocal("__int", t.hiltiType(cg))
        cg.builder().interval_as_int(tmp, e.evaluate(cg))
        return tmp

@operator.Cast(Interval, type.Float)
class _:
    def type(e, t):
        return t

    def evaluate(cg, e, t):
        tmp = cg.builder().addLocal("__double", t.hiltiType(cg))
        cg.builder().interval_as_double(tmp, e.evaluate(cg))
        return tmp

@operator.MethodCall(type.Interval, expr.Attribute("nsecs"), [])
class Begin:
    def type(obj, method, args):
        return type.UnsignedInteger(64)

    def evaluate(cg, obj, method, args):
        tmp = cg.functionBuilder().addLocal("__nsecs", hilti.type.Integer(64))
        cg.builder().interval_nsecs(tmp, obj.evaluate(cg))
        return tmp
