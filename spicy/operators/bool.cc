
opBegin(bool_::Equal)
    opOp1(std::make_shared<type::Bool>());
    opOp2(std::make_shared<type::Bool>());

    opDoc("Compares two boolean values, returning ``True`` if they are equal.");

    opValidate()
    {
        sameType(op1()->type(), op2()->type());
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(bool_::Not)
    opOp1(std::make_shared<type::Bool>());

    opDoc("Negates a boolean value.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(bool_::LogicalAnd)
    opOp1(std::make_shared<type::Bool>());
    opOp2(std::make_shared<type::Bool>());

    opDoc("Returns the logical \"and\" of two booleans.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(bool_::LogicalOr)
    opOp1(std::make_shared<type::Bool>());
    opOp2(std::make_shared<type::Bool>());

    opDoc("Returns the logical \"or\" of two booleans.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd
