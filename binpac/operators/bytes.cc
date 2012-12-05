
opBegin(bytes::Equal)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::Bytes>())

    opDoc("Compares two ``bytes`` values.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(bytes::Plus)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::Bytes>())

    opDoc("Concatenates two ``bytes`` values.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bytes>();
    }
opEnd

opBegin(bytes::PlusAssign)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::Bytes>())

    opDoc("Appends a ``bytes`` value to another one.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bytes>();
    }
opEnd


opBegin(bytes::Size)
    opOp1(std::make_shared<type::Bytes>())

    opDoc("Returns the length of the ``bytes`` instance.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd

opBegin(bytes::Match : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("match")))
    opCallArg1("r", std::make_shared<type::RegExp>())
    opCallArg2("n", std::make_shared<type::OptionalArgument>(std::make_shared<type::Integer>()))

    opDoc("Matches the ``bytes`` object against the regular expression *r*. Returns the matching part, or if *n* is given the corresponding subgroup within *r*.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd

opBegin(bytes::StartsWith : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("startswith")))
    opCallArg1("b", std::make_shared<type::Bytes>())

    opDoc("Returns true if the ``bytes`` objects begins with *b*.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(bytes::Begin : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("begin")))

    opDoc("Returns an iterator pointing to the initial element.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::iterator::Bytes>();
    }
opEnd

opBegin(bytes::End : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("end")))

    opDoc("Returns an iterator pointing one beyond the last element.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::iterator::Bytes>();
    }
opEnd



