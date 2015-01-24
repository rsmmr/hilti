
static const string _doc_connect =
   R"(
    Connects a parsing unit to a sink. All subsequent write() calls will
    pass their data to this parsing unit. Each unit can only be connected to a
    single sink. If the unit is already connected, a ~~UnitAlreadyConnected
    excpetion is thrown. However, a sink can have more than one unit connected.
   )";

opBegin(sink::Connect : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("connect")))
    opCallArg1("u", std::make_shared<type::Unit>())

    opDoc(_doc_connect)

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd


static const string _doc_connect_mime_type =
   R"(
    Connects a parsing units to a sink based on their MIME type, given as
    the argument. It works similar to ~~connect, but it instantiates and connects all
    parsers they specify the given MIME type via their ``%mimetype`` property.
    This may be zero, one, or more; which will all be connected to the sink.
   )";

opBegin(sink::ConnectMimeTypeBytes : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("connect_mime_type")))
    opCallArg1("b", std::make_shared<type::Bytes>())

    opDoc(_doc_connect)

    opValidate() {
        type_list args = { std::make_shared<type::Bytes>() };
        checkCallArgs(op3(), args);
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

opBegin(sink::ConnectMimeTypeString : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("connect_mime_type")))
    opCallArg1("b", std::make_shared<type::String>())

    opDoc(_doc_connect)

    opValidate() {
        type_list args = { std::make_shared<type::String>() };
        checkCallArgs(op3(), args);
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

static const string _doc_connect_mime_type_try =
   R"(
    Similar to ~~connect_mime_type, but connects parsers in "dynamic format detection" mode,
    in which they disable themselves gracefully if they decide they cannot parse the input.
   )";

opBegin(sink::TryConnectMimeTypeBytes : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("try_connect_mime_type")))
    opCallArg1("b", std::make_shared<type::Bytes>())

    opDoc(_doc_connect)

    opValidate() {
        type_list args = { std::make_shared<type::Bytes>() };
        checkCallArgs(op3(), args);
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

opBegin(sink::TryConnectMimeTypeString : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("try_connect_mime_type")))
    opCallArg1("b", std::make_shared<type::String>())

    opDoc(_doc_connect)

    opValidate() {
        type_list args = { std::make_shared<type::String>() };
        checkCallArgs(op3(), args);
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

static const string _doc_write =
   R"(
    Passes data on to all connected parsing units. Multiple write() calls
    act like passing incremental input in, the units parse them as if it were
    a single stream of data. If no units are connected, the call does not
    have any effect. If one parsing unit throws an exception, parsing of
    subsequent units does not proceed. Note that the order in which the data is
    parsed to which unit is undefined.

    Todo: The exception semantics are quite fuzzy. What's the right strategy
    here?
   )";

opBegin(sink::Write : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("write")))
    opCallArg1("b", std::make_shared<type::Bytes>())

    opDoc(_doc_write)

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

static const string _doc_close =
   R"(
    Closes a sink by disconnecting all parsing units. Afterwards, the
    sink's state is as if it had just been created (so new units can be
    connected). Note that a sink it automatically closed when the unit it is
    part of is done parsing. Also note that a previously connected parsing
    unit can *not* be reconnected; trying to do so will still thrown an
    ~~UnitAlreadyConnected exception.
   )";

opBegin(sink::Close : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("close")))

    opDoc(_doc_close)

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

static const string _doc_add_filter =
   R"(
    Adds an input filter as specificed by *t* (of type ~~BinPAC::Filter) to the
    sink. The filter will receive all input written into the sink first,
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
   )";

opBegin(sink::AddFilter : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("add_filter")))
    opCallArg1("t", std::make_shared<type::Enum>())

    opDoc(_doc_add_filter)

    opValidate() {
        // TODO: Check enum for actual type.
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

opBegin(sink::Size)
    opOp1(std::make_shared<type::Sink>())

    opDoc("Returns the number of bytes written into the sink so far. If the sink has filters attached, this returns the value after filtering.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd

