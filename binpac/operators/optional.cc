
opBegin(optional::CoerceOptional : Coerce)
    opOp1(std::make_shared<type::Optional>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Optional>()));

    opDoc(
        "Coerces an optional value into another optional, using the wrapped types coercion rules.");

    opMatch()
    {
        auto t1 = ast::checkedCast<type::Optional>(op1()->type());
        auto ttype = ast::checkedCast<type::TypeType>(op2()->type())->typeType();
        auto t2 = ast::checkedCast<type::Optional>(ttype);

        // TODO: Don't have access to canCoerceTo() here right now.
        // return canCoerceTo(t1->argType(), t2->argType());
        return true;
    }

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(optional::CoerceUnwrap : Coerce)
    opOp1(std::make_shared<type::Optional>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Any>()));

    opDoc("Coerces an optional value into a value of the wrapped type.");

    opMatch()
    {
        auto t1 = ast::checkedCast<type::Optional>(op1()->type());
        auto ttype = ast::checkedCast<type::TypeType>(op2()->type())->typeType();
        auto t2 = ast::checkedCast<Type>(ttype);

        if ( ! t1->argType() )
            return false;

        // TODO: Don't have access to canCoerceTo() here right now.
        // return canCoerceTo(t1->argType(), t2);
        return true;
    }

    opValidate()
    {
    }

    opResult()
    {
        auto t1 = ast::checkedCast<type::Optional>(op1()->type());
        return t1->argType();
    }

opEnd

opBegin(optional::CoerceWrap : Coerce)
    opOp1(std::make_shared<type::Any>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Optional>()));

    opDoc("Coerces an value into an optional wrapping its type.");

    opMatch()
    {
        return false;
        auto t1 = ast::tryCast<type::Optional>(op1()->type());

        if ( t1 )
            // All, except optional, which we handle separately.
            return false;

        auto ttype = ast::checkedCast<type::TypeType>(op2()->type())->typeType();
        auto t2 = ast::checkedCast<type::Optional>(ttype);

        // TODO: Don't have access to canCoerceTo() here right now.
        return sameType(t1, t2->argType());
    }

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Optional>(op1()->type());
    }

opEnd
