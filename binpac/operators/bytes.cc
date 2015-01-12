
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
        return std::make_shared<type::Bytes>();
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

opBegin(bytes::Upper : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("upper")))

    opDoc("Returns an upper-case version.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bytes>();
    }
opEnd

opBegin(bytes::Lower : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("lower")))

    opDoc("Returns a lower-case version.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bytes>();
    }
opEnd

opBegin(bytes::Strip : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("strip")))
    opCallArg1("side", std::make_shared<type::OptionalArgument>(std::make_shared<type::Enum>()))
    opCallArg2("chars", std::make_shared<type::OptionalArgument>(std::make_shared<type::Bytes>()))

    opDoc("Strips off leading and/or trailing characters, as indicated by *side* with either if not given. By default it strips off all whitespace; alternatively any characters contained in *chars*.")

    opValidate() {
        // TODO: Check enum type.
    }

    opResult() {
        return std::make_shared<type::Bytes>();
    }
opEnd

opBegin(bytes::ToUInt : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("to_uint")))
    opCallArg1("base", std::make_shared<type::OptionalArgument>(type::Integer::unsignedInteger(64)))

    opDoc("Interprets the ``bytes`` as representing an ASCII-encoded number and converts it into an unsigned integer, using a base of *base*. If *base* is not given, the default is 10.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd

opBegin(bytes::ToUIntBinary : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("to_uint")))
    opCallArg1("byte_order", std::make_shared<type::Enum>())

    opDoc("Interprets the ``bytes`` as representing an binary number encoded with the given byte order, and converts it into an unsigned integer.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd

opBegin(bytes::ToInt : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("to_int")))
    opCallArg1("base", std::make_shared<type::OptionalArgument>(type::Integer::unsignedInteger(64)))

    opDoc("Interprets the ``bytes`` as representing an ASCII-encoded number and converts it into a signed integer, using a base of *base*. If *base* is not given, the default is 10.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>(64, true);
    }
opEnd

opBegin(bytes::ToIntBinary : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("to_int")))
    opCallArg1("byte_order", std::make_shared<type::Enum>())

    opDoc("Interprets the ``bytes`` as representing an binary number encoded with the given byte order, and converts it into a signed integer.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>(64, true);
    }
opEnd

opBegin(bytes::ToTime : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("to_time")))
    opCallArg1("base", std::make_shared<type::OptionalArgument>(type::Integer::unsignedInteger(64)))

    opDoc("Interprets the ``bytes`` as representing a number of seconds since the epoch in the form of an ASCII-encoded number and converts it into a time value, using a base of *base*. If *base* is not given, the default is 10.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Time>();
    }
opEnd

opBegin(bytes::ToTimeBinary : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("to_time")))
    opCallArg1("byte_order", std::make_shared<type::Enum>())

    opDoc("Interprets the ``bytes`` as representing as number of seconds since the epoch in the form of an binary number encoded with the given byte order, and converts it into a time value.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Time>();
    }
opEnd

opBegin(bytes::Decode : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("decode")))
    opCallArg1("charset", std::make_shared<type::Enum>())

    opDoc("Interprets the ``bytes`` as representing an binary string encoded with the given character set, and converts it into a UTF8 string")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::String>();
    }
opEnd

opBegin(bytes::Split1 : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("split1")))
    opCallArg1("sep", std::make_shared<type::OptionalArgument>(std::make_shared<type::Bytes>()))

    opDoc("Splits at the first occurence of ``sep``, returning a pair of ``bytes`` representing everything before and afterwards, respectively. If *sep* is skipped, the default is to split at any sequence of white-space.")

    opValidate() {
    }

    opResult() {
        type_list t = { std::make_shared<type::Bytes>(), std::make_shared<type::Bytes>() };
        return std::make_shared<type::Tuple>(t);
    }
opEnd

opBegin(bytes::Split : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("split")))
    opCallArg1("sep", std::make_shared<type::OptionalArgument>(std::make_shared<type::Bytes>()))

    opDoc("Splits at each occurence of ``sep``, returning a vector of ``bytes`` representing each piece excluding the separators. If *sep* is skipped, the default is to split at any sequence of white-space.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Vector>(std::make_shared<type::Bytes>());
    }
opEnd

opBegin(bytes::Join : MethodCall)
    opOp1(std::make_shared<type::Bytes>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("join")))
    opCallArg1("l", std::make_shared<type::List>())

    opDoc("Renders the elements of *l* into textual form and joins them into a single bytes object using the given one as separator.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bytes>();
    }
opEnd


