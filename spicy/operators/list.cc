
opBegin(list::PlusAssign)
    opOp1(std::make_shared<type::List>());
    opOp2(std::make_shared<type::List>());

    opDoc("Appends a lsit value to another one.");

    opValidate()
    {
        sameType(op1()->type(), op2()->type());
    }

    opResult()
    {
        return op1()->type();
    }
opEnd

opBegin(list::PushBack : MethodCall)
    opOp1(std::make_shared<type::List>());
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("push_back")));
    opCallArg1("elem", std::make_shared<type::Any>());

    opDoc("Appends an element to the list.");

    opValidate()
    {
        // TODO: Check element ytpe.
    }

    opResult()
    {
        return op1()->type();
    }
opEnd

opBegin(list::Size)
    opOp1(std::make_shared<type::List>());

    opDoc("Returns the length of the list.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd
