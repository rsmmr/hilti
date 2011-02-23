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
        return cg.functionBuilder().typeByID("BinPAC::Sink")

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
        old_sink = cg.builder().addLocal("__old_sink", sink.type())
        old_sink_set = cg.builder().addLocal("__old_sink_set", hilti.type.Bool())

        cg.builder().struct_get(old_sink, pobj, "__sink")
        cg.builder().ref_cast_bool(old_sink_set, old_sink)

        fbuilder = cg.functionBuilder()
        ok = fbuilder.newBuilder("not_connected")
        error = fbuilder.newBuilder("already_connected")
        cg.builder().if_else(old_sink_set, error, ok)

        error.makeRaiseException("BinPAC::UnitAlreadyConnected", None)

        cg.setBuilder(ok)

        parser = cg.builder().addLocal("__parser", cg.functionBuilder().typeByID("BinPAC::Parser"))
        cg.builder().struct_get(parser, pobj, "__parser")

        cfunc = cg.builder().idOp("BinPAC::sink_connect")
        cargs = cg.builder().tupleOp([sink, pobj, parser])
        cg.builder().call(None, cfunc, cargs)

        # Record the connected sink in parsing object.
        cg.builder().struct_set(pobj, "__sink", sink)

        return None

@operator.MethodCall(type.Sink, expr.Attribute("connect_mime_type"), [type.String])
class _:
    """Connects a parsing units to a sink based on their MIME type, given as
    *op1*. It works similar to ~~connect, but it instantiates and connects all
    parsers they specify the given MIME type via their ``%mimetype`` property.
    This may be zero, one, or more; which will all be connected to the sink.
    """
    def type(obj, method, args):
        return type.Void()

    def evaluate(cg, obj, method, args):
        sink = obj.evaluate(cg)
        mtype = args[0].evaluate(cg)
        cfunc = cg.builder().idOp("BinPAC::mime_connect_by_string")
        cargs = cg.builder().tupleOp([sink, mtype])
        cg.builder().call(None, cfunc, cargs)
        return None

@operator.MethodCall(type.Sink, expr.Attribute("connect_mime_type"), [type.Bytes])
class _:
    """Connects a parsing units to a sink based on their MIME type, given as
    *op1*. It works similar to ~~connect, but it instantiates and connects all
    parsers they specify the given MIME type via their ``%mimetype`` property.
    This may be zero, one, or more; which will all be connected to the sink.
    """
    def type(obj, method, args):
        return type.Void()

    def evaluate(cg, obj, method, args):
        sink = obj.evaluate(cg)
        mtype = args[0].evaluate(cg)
        cfunc = cg.builder().idOp("BinPAC::mime_connect_by_bytes")
        cargs = cg.builder().tupleOp([sink, mtype])
        cg.builder().call(None, cfunc, cargs)
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
        user = cg.builder().idOp("__user")
        cfunc = cg.builder().idOp("BinPAC::sink_write")
        cargs = cg.builder().tupleOp([sink, data, user])
        cg.builder().call(None, cfunc, cargs)

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
        cfunc = cg.builder().idOp("BinPAC::sink_close")
        cargs = cg.builder().tupleOp([sink])
        cg.builder().call(None, cfunc, cargs)
        return None

@operator.MethodCall(type.Sink, expr.Attribute("add_filter"), [type.Enum])
class _:
    """Adds an input filter of type *op1* (of type ~~BinPAC::Filter) to the
    sink. *op1* The filter will receive all input written into the sink first,
    transform it according to its semantics, and then parser attached to the
    unit will parse the *output* of the filter.

    Multiple filters can be added to a sink, in which case they will be
    chained into a pipeline and the data is passed through them in the order
    they have been added. The parsing will then be carried out on the output
    of the last filter in the chain.

    Note that filters must be added before the first data chunk is written
    into the sink. If data has already been written when a filter is added,
    behaviour is undefined.

    Currently, only a set of predefined filters can be used; see
    ~~BinPAC::Filter. One cannot define own filters in BinPAC++.

    Todo: We should probably either enables adding filters laters, or catch
    the case of adding them too late at run-time an abort with an exception.
    """
    def type(obj, method, args):
        return type.Void()

    def validate(vld, obj, method, args):
        # TODO: Make srue it's the right enum ...
        return

    def evaluate(cg, obj, method, args):
        sink = obj.evaluate(cg)
        ftype = args[0].evaluate(cg)

        filter = cg.builder().addLocal("__filter", cg.functionBuilder().typeByID("BinPAC::ParseFilter"))
        cg.builder().call(filter, cg.builder().idOp("BinPAC::filter_new"),  cg.builder().tupleOp([ftype]))
        cg.builder().call(None, cg.builder().idOp("BinPAC::sink_filter"), cg.builder().tupleOp([sink, filter]))

        return None

