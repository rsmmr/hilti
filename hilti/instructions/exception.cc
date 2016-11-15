
iBegin(exception::New, "new")
    iTarget(optype::refException);
    iOp1(optype::typeException, true);

    iValidate
    {
        auto type = ast::rtti::tryCast<expression::Type>(op1)->typeValue();
        auto etype = ast::rtti::tryCast<type::Exception>(type);

        if ( etype->argType() )
            error(type, ::util::fmt("exception takes an argument of type %s",
                                    etype->argType()->render().c_str()));
    }

    iDoc(R"(
        Instantiates a new exception.
    )");
iEnd

iBegin(exception::NewWithArg, "new")
    iTarget(optype::refException);
    iOp1(optype::typeException, true);
    iOp2(optype::any, true);

    iValidate
    {
        auto type = ast::rtti::tryCast<expression::Type>(op1)->typeValue();
        auto etype = ast::rtti::tryCast<type::Exception>(type);

        if ( ! etype->argType() )
            error(type, "exception does not take an argument");
        else
            canCoerceTo(op2, etype->argType());
    }

    iDoc(R"(
        Instantiates a new exception.
    )");
iEnd

iBegin(exception::Throw, "exception.throw")
    iOp1(optype::refException, false);

    iValidate
    {
    }

    iDoc(R"(
         Throws an exception, triggering the closest handler.
    )");
iEnd

iBegin(exception::__BeginHandler, "exception.__begin_handler")
    iOp1(optype::label, true);
    iOp2(optype::optional(optype::typeException), true);

    iValidate
    {
    }

    iDoc(R"(
        Internal instruction beginning scope of an exception handler. Note that
        this instruction is used statically at compile-time, not exectued at
        run-time. It is thus subject to its static location, not control flow.
    )");
iEnd

iBegin(exception::__EndHandler, "exception.__end_handler")
    iValidate
    {
    }

    iDoc(R"(
            Internal instruction ending scope of most current exception handler.
            Note that this instruction is used statically at compile-time, not
            exectued at run-time. It is thus subject to its static location, not
            control flow.
    )");
iEnd

iBegin(exception::__GetAndClear, "exception.__get_and_clear")
    iTarget(optype::refException);

    iValidate
    {
    }

    iDoc(R"(
        Internal instruction returning the currently raised exception, clearing
        it.
    )");
iEnd

iBegin(exception::__Clear, "exception.__clear")
    iValidate
    {
    }

    iDoc(R"(
        Internal instruction clearing any currently raised exception.
    )");
iEnd
