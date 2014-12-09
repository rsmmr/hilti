
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


static const string _doc_write =
   R"(
    Passes data on to all connected parsing units. Multiple write() calls
    act like passing incremental input in, the units parse them as if it were
    a single stream of data. If data is passed in out of order, it will be reassembled
    before passing on, according to the sequence number *seq* provided; *seq* is interpreted
    relative to the inital sequence number set with *set_initial_sequence_number*, or 0 if
    not otherwise set. If not sequence number is provided, the data is assumed to represent
    a chunk to be appended to the current end of the input stream.

    If no units are connected, the call does not
    have any effect. If one parsing unit throws an exception, parsing of
    subsequent units does not proceed. Note that the order in which the data is
    parsed to which unit is undefined.

    Todo: The exception semantics are quite fuzzy. What's the right strategy
    here?
   )";

static shared_ptr<binpac::Type> _makeUInt64()
{
    return std::make_shared<binpac::type::Integer>(64, false);
}

opBegin(sink::Write : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("write")))
    opCallArg1("b", std::make_shared<type::Bytes>())
    opCallArg2("seq", std::make_shared<type::OptionalArgument>(_makeUInt64()))

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

static const string _doc_sequence =
   R"(
   Returns the current sequence number of the sink's input stream, which is
   one beyond all data that has been put in order and delivered so far.

   The returned value is relative to the sink's initial sequence number,
   which defaults to zero.
   )";

opBegin(sink::Sequence : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("sequence")))

    opDoc(_doc_sequence)

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd


static const string _doc_gap =
   R"(
   Reports a gap in the input stream. *seq* is the sequence number of the
   first byte missing, *len* is the length of the gap.
   *seq* is relative to the sink's initial sequence number,
   which defaults to zero.
   )";

opBegin(sink::Gap : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("gap")))
    opCallArg1("seq", _makeUInt64())
    opCallArg2("len", _makeUInt64())

    opDoc(_doc_gap)

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

static const string _doc_skip =
   R"(
   Skips ahead in the input stream. *seq* is
   is the sequence number where to continue parsing, 
   relative to the sink's initial sequence number.
   If there's still data buffered before that position,
   that will be ignored and, if auto-skip is on, also
   immediately deleted. If new data is passed in later before *seq* that
   will likewise be ignored. If the input stream is currently stuck
   inside a gap, and *seq* is beyond that gap, the stream will resume
   processing at *seq*.
   )";

opBegin(sink::Skip : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("skip")))
    opCallArg1("seq", _makeUInt64())

    opDoc(_doc_skip)

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

static const string _doc_trim =
   R"(
   Deletes all data that's still buffered internally up to *seq*.
   *seq* is relative to the sink's initial sequence number,
   which defaults to zero.
   If processing the input stream hasn't reached
   *seq* yet, it will also skip ahead to there.

   Trimming the input stream releases the memory, but means that the sink won't
   be able to detect any further data mismatches.

   Note that by default, auto-trimming is enabled, which means all data is trimmed
   automatically once in-order and procssed.
   )";

opBegin(sink::Trim : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("trim")))
    opCallArg1("seq", _makeUInt64())

    opDoc(_doc_trim)

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

static const string _doc_set_initial_sequence_number =
   R"(
   Sets the sink's initial sequence number. All sequence numbers given to
   other methods are interpreted relative to this one. By default, a sink's
   initial sequence number is zero.
   )";

opBegin(sink::SetInitialSequenceNumber : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("set_initial_sequence_number")))
    opCallArg1("seq", _makeUInt64())

    opDoc(_doc_set_initial_sequence_number)

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

static const string _doc_set_auto_trim =
   R"(
   Enables or disables auto-trimming. If enabled (which is the default)
   sink input data is trimmed automatically once in-order and procssed. See
   \a trim()  for more information about trimming.

   TODO: Disabling auto-trimming is not yet supported.
   )";

opBegin(sink::SetAutoTrim : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("set_auto_trim")))
    opCallArg1("enabled", std::make_shared<type::Bool>())

    opDoc(_doc_set_auto_trim)

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

static const string _doc_set_policy =
   R"(
   Sets a sink's reassembly policy for ambigious input. As long as data hasn't been trimmed,
   a sink detects overlapping chunks. The policy decides how to handle ambigious overlaps.
   The default policy is \a BinPAC::ReassemblyPolicy::First, which resolved ambigiuities by taking
   the data from chunk that came first.

   TODO: \a First is currently the only policy supported.
   )";

opBegin(sink::SetPolicy : MethodCall)
    opOp1(std::make_shared<type::Sink>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("set_policy")))
    opCallArg1("policy", std::make_shared<type::Enum>())

    opDoc(_doc_set_policy)

    opValidate() {
        // TODO: Check enum for actual type.
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

