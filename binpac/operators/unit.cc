
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
        assert(i);
        assert(i->type());

        return i->type();
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
        assert(i);
        assert(i->type());

        return i->type();
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
        auto unit = ast::checkedCast<type::Unit>(op1()->type());

        auto tuple = op2();
        if ( ! tuple ) {
            expression_list types = {};
            auto empty = std::make_shared<constant::Tuple>(types);
            tuple = std::make_shared<expression::Constant>(empty);
        }

        checkCallArgs(tuple, unit->parameterTypes());
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
    opOp3(std::make_shared<type::Tuple>())

    opDoc(_doc_input)

    opValidate() {
        type_list args = {};
        checkCallArgs(op3(), args);
    }

    opResult() {
        return std::make_shared<type::iterator::Bytes>();
    }
opEnd

static const string _doc_set_input =
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

opBegin(unit::SetInput : MethodCall)
    opOp1(std::make_shared<type::Unit>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("set_input")))
    opOp3(std::make_shared<type::Tuple>())

    opDoc(_doc_set_input)

    opValidate() {
        type_list args = { std::make_shared<type::iterator::Bytes>() };
        checkCallArgs(op3(), args);
    }

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
    opOp3(std::make_shared<type::Tuple>())

    opDoc(_doc_offset)

    opValidate() {
        checkCallArgs(op3(), {});
    }

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
    opOp3(std::make_shared<type::Tuple>())

    opDoc(_doc_add_filter)

    opValidate() {
        type_list args = { std::make_shared<type::Enum>() }; // FIXME: Check for actual type.
        checkCallArgs(op3(), args);
    }

    opResult() {
        return std::make_shared<type::Void>();
    }
opEnd




