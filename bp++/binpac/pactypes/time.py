# $Id$
#
# The time type.

import binpac.type as type
import binpac.expr as expr
import binpac.operator as operator

import hilti.type

@type.pac("time")
class Time(type.Type):
    """Type for time objects.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Time, self).__init__(location=location)

    ### Overridden from Type.

    def validate(self, vld):
        return True

    def validateCtor(self, vld, ctor):
        if not (isinstance(ctor, tuple) and len(ctor) == 2) or \
           not isinstance(ctor[0], int) or \
           not isinstance(ctor[1], int):
            vld.error(ctor, "time: constant of wrong internal type")

    def hiltiType(self, cg):
        return hilti.type.Time()

    def hiltiCtor(self, cg, ctor):
        const = hilti.constant.Constant(ctor, hilti.type.Time())
        return hilti.operand.Constant(const)

    def name(self):
        return "time"

    def pac(self, printer):
        printer.output("time")

    def pacCtor(self, printer, value):
        printer.output("%d.%09d", value[0], value[1])

@operator.Plus(Time, type.Interval)
class _:
    def type(e1, e2):
        return Time()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Time())
        cg.builder().time_add(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Minus(Time, type.Interval)
class _:
    def type(e1, e2):
        return Time()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Time())
        cg.builder().time_sub(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Plus(type.Interval, Time)
class _:
    def type(e1, e2):
        return Time()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Time())
        cg.builder().time_add(tmp, e2.evaluate(cg), e1.evaluate(cg))
        return tmp

@operator.Minus(type.Interval, Time)
class _:
    def type(e1, e2):
        return Time()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Time())
        cg.builder().time_sub(tmp, e2.evaluate(cg), e1.evaluate(cg))
        return tmp

@operator.Equal(Time, Time)
class _:
    def type(e1, e2):
        return type.Bool()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Bool())
        cg.builder().time_eq(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Lower(Time, Time)
class _:
    def type(e1, e2):
        return type.Bool()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Bool())
        cg.builder().time_lt(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Greater(Time, Time)
class _:
    def type(e1, e2):
        return type.Bool()

    def evaluate(cg, e1, e2):
        tmp = cg.builder().addLocal("__result", hilti.type.Bool())
        cg.builder().time_gt(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp

@operator.Coerce(type.Float, Time)
class _:
    def coerceCtorTo(ctor, dsttype):
        return ctor

    def coerceTo(cg, e, dsttype):
        tmp = cg.builder().addLocal("__time", hilti.type.Time())
        cg.builder().float_as_time(tmp, e.evaluate(cg))
        return tmp

@operator.Coerce(type.UnsignedInteger, Time)
class _:
    def coerceCtorTo(ctor, dsttype):
        return (ctor, 0)

    def coerceTo(cg, e, dsttype):
        tmp = cg.builder().addLocal("__time", hilti.type.Time())
        cg.builder().int_as_time(tmp, e.evaluate(cg))
        return tmp

@operator.Cast(Time, type.UnsignedInteger)
class _:
    def type(e, t):
        return t

    def evaluate(cg, e, t):
        tmp = cg.builder().addLocal("__int", t.hiltiType(cg))
        cg.builder().time_as_int(tmp, e.evaluate(cg))
        return tmp

@operator.Cast(Time, type.Float)
class _:
    def type(e, t):
        return t

    def evaluate(cg, e, t):
        tmp = cg.builder().addLocal("__double", t.hiltiType(cg))
        cg.builder().time_as_double(tmp, e.evaluate(cg))
        return tmp

@operator.MethodCall(type.Time, expr.Attribute("nsecs"), [])
class Begin:
    def type(obj, method, args):
        return type.UnsignedInteger(64)

    def evaluate(cg, obj, method, args):
        tmp = cg.functionBuilder().addLocal("__nsecs", hilti.type.Integer(64))
        cg.builder().time_nsecs(tmp, obj.evaluate(cg))
        return tmp
