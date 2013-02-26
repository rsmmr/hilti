
opBegin(address::Equal)
    opOp1(std::make_shared<type::Address>())
    opOp2(std::make_shared<type::Address>())

    opDoc("Compares two address values, returning ``True`` if they are equal.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(address::Family : MethodCall)
    opOp1(std::make_shared<type::Address>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("family")))

    opDoc("Returns the IP family of an address value.")
    opDocResult(std::make_shared<type::TypeByName>("binpac::AddrFamily"));

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Unknown>(std::make_shared<ID>("BinPAC::AddrFamily"));
    }
opEnd

