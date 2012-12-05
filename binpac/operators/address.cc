
opBegin(address::Equal)
    opOp1(std::make_shared<type::Address>())
    opOp2(std::make_shared<type::Address>())

    opDoc("Compares two address values.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(address::Family : MethodCall)
    opOp1(std::make_shared<type::Address>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("family")))

    opDoc("Returns an address' family.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Unknown>(std::make_shared<ID>("BinPAC::AddrFamily"));
    }
opEnd

