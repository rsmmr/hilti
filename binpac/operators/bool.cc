
opBegin(bool_::Equal)
    opOp1(std::make_shared<type::Bool>())
    opOp2(std::make_shared<type::Bool>())

    opDoc("Compares two enum values.")

    opValidate() {
        sameType(op1()->type(), op2()->type());
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(bool_::Not)
    opOp1(std::make_shared<type::Bool>())

    opDoc("Negates a boolean value.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(bool_::LogicalAnd)
    opOp1(std::make_shared<type::Bool>())
    opOp2(std::make_shared<type::Bool>())

    opDoc("Logical 'and'.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(bool_::LogicalOr)
    opOp1(std::make_shared<type::Bool>())
    opOp2(std::make_shared<type::Bool>())

    opDoc("Logical 'or'.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

