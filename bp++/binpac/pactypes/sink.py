#
# The sink type.
#

import binpac.type as type
import binpac.expr as expr
import binpac.grammar as grammar
import binpac.operator as operator

import hilti.type

@type.pac("sink")
class Sink(type.Type):
    """Type for sink objects.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Sink, self).__init__(location=location)

    def hiltiType(self, cg):
        return cg.functionBuilder().typeByID("BinPACIntern::Sink")

    def pac(self, printer):
        printer.output("sink")

@operator.MethodCall(type.Sink, expr.Attribute("connect"), [type.Unit])
class _:
    """Connects a parsing unit to a sink. All subsequent write() call will
    pass their data to this parsing unit. Each unit can only be connected to a
    single sink. If the unit is already connected, a ~~UnitAlreadyConnected
    excpetion is thrown.
    """
    def type(obj, method, args):
        return type.Void()

    def evaluate(cg, obj, method, args):
        sink = obj.evaluate(cg)
        pobj = args[0].evaluate(cg)

        # Check if it's already connected.
        old_sink_set = cg.builder().addLocal("__old_sink_set", hilti.type.Bool())
        cg.builder().struct_is_set(old_sink_set, pobj, cg.builder().constOp("__sink"))

        fbuilder = cg.functionBuilder()
        ok = fbuilder.newBuilder("not_connected")
        error = fbuilder.newBuilder("already_connected")
        cg.builder().if_else(old_sink_set, error.labelOp(), ok.labelOp())

        error.makeRaiseException("BinPAC::UnitAlreadyConnected", None)

        cg.setBuilder(ok)

        parser = cg.builder().addLocal("__parser", cg.functionBuilder().typeByID("BinPACIntern::Parser"))
        cg.builder().struct_get(parser, pobj, cg.builder().constOp("__parser"))

        cfunc = cg.builder().idOp("BinPACIntern::sink_connect")
        cargs = cg.builder().tupleOp([sink, pobj, parser])
        cg.builder().call(None, cfunc, cargs)

        # Record the connected sink in parsing object.
        cg.builder().struct_set(pobj, cg.builder().constOp("__sink"), sink)

        return None

@operator.MethodCall(type.Sink, expr.Attribute("write"), [type.Bytes])
class _:
    """Passes data on to all connected parsing units. Multiple write() calls
    act like passing incremental input in, the units parse them as if it were
    a single stream of data. If not units are connected, the call does not
    have any effect. If one parsing unit throws an exception, parsing of
    subsequent units is not pursued. Note that the order in which the data is
    parsed to units is undefined.

    Todo: The exception semantics are quite fuzzy. What's the right strategy
    here?
    """
    def type(obj, method, args):
        return type.Void()

    def evaluate(cg, obj, method, args):
        sink = obj.evaluate(cg)
        data = args[0].evaluate(cg)
        cfunc = cg.builder().idOp("BinPACIntern::sink_write")
        cargs = cg.builder().tupleOp([sink, data])
        cg.builder().call(None, cfunc, cargs)
        return None

@operator.MethodCall(type.Sink, expr.Attribute("close"), [])
class _:
    """Closes a sink by disconnecting all parsing units. Afterwards, the
    sink's state as if it had just been created (so new units can be
    connected). Note that a sink it automatically closed when the unit it is
    part of is done parsing. Also note that a previously connected parsing
    unit can *not* be reconnected; trying to do so will still thrown an
    ~~UnitAlreadyConnected exception.
    """
    def type(obj, method, args):
        return type.Void()

    def evaluate(cg, obj, method, args):
        sink = obj.evaluate(cg)
        cfunc = cg.builder().idOp("BinPACIntern::sink_close")
        cargs = cg.builder().tupleOp([sink])
        cg.builder().call(None, cfunc, cargs)
        return None


