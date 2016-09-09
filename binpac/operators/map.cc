
opBegin(map::In)
    opOp1(std::make_shared<type::Any>());
    opOp2(std::make_shared<type::Map>());

    opDoc("Returns true if there's a map element with the given index.");

    opValidate()
    {
        auto mtype = ast::rtti::checkedCast<type::Map>(op2()->type());
        canCoerceTo(op1(), mtype->keyType());
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(map::Delete)
    opOp1(std::make_shared<type::Map>());
    opOp2(std::make_shared<type::Any>());

    opDoc("Deletes an element from the map. If the element does not exist, there's no effect.");

    opValidate()
    {
        auto mtype = ast::rtti::checkedCast<type::Map>(op1()->type());
        canCoerceTo(op2(), mtype->keyType());
    }

    opResult()
    {
        return std::make_shared<type::Void>();
    }
opEnd

opBegin(map::Index)
    opOp1(std::make_shared<type::Map>());
    opOp2(std::make_shared<type::Any>());

    opDoc("Returns the map element at the given index.");

    opValidate()
    {
        auto mtype = ast::rtti::checkedCast<type::Map>(op1()->type());
        canCoerceTo(op2(), mtype->keyType());
    }

    opResult()
    {
        auto mtype = ast::rtti::checkedCast<type::Map>(op1()->type());
        return mtype->valueType();
    }
opEnd

opBegin(map::IndexAssign)
    opOp1(std::make_shared<type::Map>());
    opOp2(std::make_shared<type::Any>());
    opOp3(std::make_shared<type::Any>());

    opDoc(
        "Assigns an element to the given index of the map. Any already existing element will be "
        "overwritten.");

    opValidate()
    {
        auto mtype = ast::rtti::checkedCast<type::Map>(op1()->type());
        canCoerceTo(op2(), mtype->keyType());
        canCoerceTo(op3(), mtype->valueType());
    }

    opResult()
    {
        auto v = ast::rtti::checkedCast<type::Map>(op1()->type());
        return std::make_shared<type::Void>();
    }
opEnd

opBegin(map::Size)
    opOp1(std::make_shared<type::Map>());

    opDoc("Returns the number of elements in the map.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd

opBegin(map::Get : MethodCall)
    opOp1(std::make_shared<type::Map>());
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("get")));
    opCallArg1("index", std::make_shared<type::Any>());
    opCallArg2("default", std::make_shared<type::OptionalArgument>(std::make_shared<type::Any>()));

    opDoc(
        "Returns the map element at the given *index*. If the element does not exist, *default* is "
        "returned if given.");

    opValidate()
    {
        auto mtype = ast::rtti::checkedCast<type::Map>(op1()->type());
        type_list args = {mtype->keyType(),
                          std::make_shared<type::OptionalArgument>(mtype->valueType())};
        checkCallArgs(op3(), args);
    }

    opResult()
    {
        auto mtype = ast::rtti::checkedCast<type::Map>(op1()->type());
        return mtype->valueType();
    }
opEnd

opBegin(map::Clear : MethodCall)
    opOp1(std::make_shared<type::Map>());
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("clear")));

    opDoc("Removes all elements from the map.");

    opValidate()
    {
    }

    opResult()
    {
        return op1()->type();
    }
opEnd
