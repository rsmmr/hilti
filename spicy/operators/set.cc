
opBegin(set::In)
    opOp1(std::make_shared<type::Any>());
    opOp2(std::make_shared<type::Set>());

    opDoc("Returns true if the element is a member of the set.");

    opValidate()
    {
        matchesElementType(op1(), op2()->type());
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(set::Add)
    opOp1(std::make_shared<type::Set>());
    opOp2(std::make_shared<type::Any>());

    opDoc("Adds an element to the set.");

    opValidate()
    {
        matchesElementType(op2(), op1()->type());
    }

    opResult()
    {
        return std::make_shared<type::Void>();
    }
opEnd

opBegin(set::Delete)
    opOp1(std::make_shared<type::Set>());
    opOp2(std::make_shared<type::Any>());

    opDoc("Deletes an element from the set. If the element does not exist, there's no effect.");

    opValidate()
    {
        matchesElementType(op2(), op1()->type());
    }

    opResult()
    {
        return std::make_shared<type::Void>();
    }
opEnd

opBegin(set::Size)
    opOp1(std::make_shared<type::Set>());

    opDoc("Returns the number of elements in the set.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd

opBegin(set::Clear : MethodCall)
    opOp1(std::make_shared<type::Set>());
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("clear")));

    opDoc("Removes all elements from the set.");

    opValidate()
    {
    }

    opResult()
    {
        return op1()->type();
    }
opEnd
