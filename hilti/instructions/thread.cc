
iBegin(thread::GetContext, "thread.get_context")
    iTarget(optype::refContext);

    iValidate
    {
        // Checked in passes::Validator.
    }

    iDoc(R"(
        Returns the thread context of the currently executing virtual thread.
        The type of the result operand must opf type ``context``, which is
        implicitly defined as current function's thread context type. The
        instruction cannot be used if no such has been defined.
    )")
iEnd

iBegin(thread::ThreadID, "thread.id")
    iTarget(optype::int64);

    iValidate
    {
    }

    iDoc(R"(
        Returns the ID of the current virtual thread. Returns -1 if executed in
        the primary thread.
    )");
iEnd

iBegin(thread::Schedule, "thread.schedule")
    iOp1(optype::function, true);
    iOp2(optype::tuple, true);
    iOp3(optype::optional(optype::int64), true);

    iValidate
    {
        auto ftype = ast::rtti::checkedCast<type::Function>(op1->type());
        auto rtype = ftype->result()->type();
        shared_ptr<Type> none = nullptr;
        checkCallParameters(ftype, op2);

        if ( ! ast::rtti::isA<type::Void>(rtype) ) {
            error(op1, "function must not return a value");
        }

        // Scope promotion is checked in validator.
    }

    iDoc(R"(
        Schedules a function call onto a virtual thread. If *op3* is not given,
        the current thread context determines the target thread, according to
        HILTI context-based scheduling. If *op3* is given, it gives the target
        thread ID directly; in this case the functions thread context will be
        cleared when running.
    )");
iEnd

iBegin(thread::SetContext, "thread.set_context")
    iOp1(optype::any, true);

    iValidate
    {
        // Checked in passes::Validator.
    }

    iDoc(R"(
        Sets the thread context of the currently executing virtual thread to
        *op1*. The type of *op1* must match the current module's ``context``
        definition.
    )");
iEnd
