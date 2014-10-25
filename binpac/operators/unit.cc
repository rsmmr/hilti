
opBegin(unit::Attribute)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>())

    opDoc("Access a unit field.")

    opValidate() {
        auto unit = ast::checkedCast<type::Unit>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        if ( ! unit->item(attr->id()) )
            error(op2(), "unknown unit item");
    }

    opResult() {
        auto unit = ast::checkedCast<type::Unit>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        auto i = unit->item(attr->id());
        if ( ! i )
            // Error will be reported by validation.
            return std::make_shared<type::Unknown>();

        assert(i->fieldType());
        return i->fieldType();
    }
opEnd

opBegin(unit::HasAttribute)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>())

    opDoc("Returns true if a unit field is set.")

    opValidate() {
        auto unit = ast::checkedCast<type::Unit>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        if ( ! unit->item(attr->id()) )
            error(op2(), "unknown unit item");
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(unit::TryAttribute)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>())

    opDoc("Returns the value of a unit field if it's set; otherwise throws an BinPAC::AttributeNotSet exception.")

    opValidate() {
        auto unit = ast::checkedCast<type::Unit>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        if ( ! unit->item(attr->id()) )
            error(op2(), "unknown unit item");
    }

    opResult() {
        auto unit = ast::checkedCast<type::Unit>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        auto i = unit->item(attr->id());
        if ( ! i )
            // Error will be reported by validation.
            return std::make_shared<type::Unknown>();

        assert(i->fieldType());
        return i->fieldType();
    }
opEnd

opBegin(unit::AttributeAssign)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>())
    opOp3(std::make_shared<type::Any>())

    opDoc("Assign a value to a unit field.")

    opValidate() {
        auto unit = ast::checkedCast<type::Unit>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        if ( ! unit->item(attr->id()) )
            error(op2(), "unknown unit item");
    }

    opResult() {
        auto unit = ast::checkedCast<type::Unit>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        auto i = unit->item(attr->id());

        if ( ! i )
            // Error will be reported by validation.
            return std::make_shared<type::Unknown>();

        assert(i->fieldType());

        return i->fieldType();
    }
opEnd

opBegin(unit::New)
    opOp1(std::make_shared<type::TypeType>())
    opOp2(std::make_shared<type::OptionalArgument>(std::make_shared<type::Tuple>()))

    opDoc("Instantiates a new parse object for a given unit type.")

    opMatch() {
        auto type = ast::checkedCast<type::TypeType>(op1()->type())->typeType();
        return ast::isA<type::Unit>(type);
    }

    opValidate() {
        auto ttype = ast::checkedCast<type::TypeType>(op1()->type());
        auto unit = ast::checkedCast<type::Unit>(ttype->typeType());

        if ( hasOp(2) )
            checkCallArgs(op2(), unit->parameterTypes());

        else
            checkCallArgs(nullptr, unit->parameterTypes());
    }

    opResult() {
        auto type = ast::checkedCast<type::TypeType>(op1()->type())->typeType();
        return type;
    }
opEnd

static const string _doc_input =
   R"(
    Returns an ``iter<bytes>`` referencing the first byte of the raw data
    for parsing the unit. This method must only be called while the unit is
    being parsed, and will throw an ``UndefinedValue`` exception otherwise.

    Note that using this method requires the unit being parsed to fully buffer
    its input until finished. That may have a performance impact, in
    particular in terms of memory requirements since now the garbage
    collection may need to hold on to it significantly longer.
   )";

opBegin(unit::Input : MethodCall)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("input")))

    opDoc(_doc_input)

    opResult() {
        return std::make_shared<type::iterator::Bytes>();
    }
opEnd

static const string _doc_set_position =
   R"(
    Changes the position in the input stream to continue parsing from. The
    new position is a new ``iter<bytes>`` where subsequent parsing will
    proceed. Note this changes the position *globally*: all subsequent field
    will be parsed from the new position, including those of a potential
    higher-level unit this unit is part of. Returns an ``iter<bytes>`` with
    the old position.

    Note that using this method requires the unit being parsed to fully buffer
    its input until finished. That may have a performance impact, in
    particular in terms of memory requirements since now the garbage
    collection may need to hold on to it significantly longer.
   )";

opBegin(unit::SetPosition : MethodCall)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("set_position")))
    opCallArg1("b", std::make_shared<type::iterator::Bytes>())

    opDoc(_doc_set_position)

    opResult() {
        return std::make_shared<type::iterator::Bytes>();
    }
opEnd

static const string _doc_offset =
   R"(
    Returns the an \c uint<64> offset of the current parsing position relative to the
    start of the current parsing unit. This method must only be called while
    the unit is being parsed, and will throw an ``UndefinedValue`` exception
    otherwise.

    Note that when being inside a field hook, the current parsing position
    will have already moved on to the start of the *next* field because the
    hook is only run after the current field has been fully parsed. On the
    other hand, if the method is called from an expression evaluated before
    the parsing of a field starts (such as in a field's ``&length``
    attribute), the returned offset will reflect the beginning of that field.

    Note that using this method requires the unit being parsed to fully buffer
    its input until finished. That may have a performance impact, in
    particular in terms of memory requirements since now the garbage
    collection may need to hold on to it significantly longer.
   )";

opBegin(unit::Offset : MethodCall)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("offset")))

    opDoc(_doc_offset)

    opResult() {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd

static const string _doc_add_filter =
   R"(
    Adds an input filter of type ~~BinPAC::Filter to the
    unit object. The filter will receive all parsed input first,
    transform it according to its semantics, and then the unit will parse the
    *output* of the filter.

    Multiple filters can be added to a parsing unit, in which case they will
    be chained into a pipeline and the data is passed through them in the
    order they have been added. The actual unit parsing will then be carried
    out on the output of the last filter in the chain.

    Note that filters must be added before the first data chunk is passed in.
    If parsing has alrady started when a filter is added, behaviour is
    undefined.  Also note that filters can only be added to *exported* unit
    types.

    Currently, only a set of predefined filters can be used; see
    ~~BinPAC::Filter. One cannot define own filters in BinPAC++ (but one can
    achieve a similar effect with sinks.)

    Todo: We should probably either enables adding filters laters, or catch
    the case of adding them too late at run-time and abort with an exception.
   )";

opBegin(unit::AddFilter : MethodCall)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("add_filter")))
    opCallArg1("f", std::make_shared<type::Enum>())

    opDoc(_doc_add_filter)

    opValidate() {
        // FIXME: Check for actual type.
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

static const string _doc_disconnect =
   R"(
    Disconnect the unit from its parent sink. The unit gets signaled a
    regular end of data, so if it still has input pending, that might be
    processed before the method returns.  If the unit is not connected to a
    sink, the method does not have any effect.
   )";

opBegin(unit::Disconnect : MethodCall)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("disconnect")))

    opDoc(_doc_disconnect)

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd

static const string _doc_mime_type =
   R"(
   Returns the MIME type that was specified when the unit was
   instantiated (e.g., via ~~sink.connect_mime_type()). Returns an
   empty ``bytes`` object if none was specified. This method can
   only be called for exported types.
   )";

opBegin(unit::MimeType : MethodCall)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("mime_type")))

    opDoc(_doc_mime_type)

    opResult() {
        return std::make_shared<type::Bytes>();
    }
opEnd

static const string _doc_backtrack =
   R"(
       Abort parsing at the current position and returns back to the most revent ``&try`` attribute.
       Turns into a parse error if there's no ``&try``.
   )";

opBegin(unit::Backtrack : MethodCall)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("backtrack")))

    opDoc(_doc_backtrack)

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd



