
opBegin(bool_::Not)
    opOp1(std::make_shared<type::Bool>())

    opDoc("Negates a boolean value.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

