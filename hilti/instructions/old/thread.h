iBegin(thread, GetContext, "thread.get_context")
    iTarget(optype::ref\ <Context>)

    iValidate {
        auto ty_target = as<type::ref\ <Context>>(target->type());

    }

    iDoc(R"(    
        Returns the thread context of the currently executing virtual thread.
        The type of the result operand must opf type ``context``, which is
        implicitly defined as current function's thread context type. The
        instruction cannot be used if no such has been defined.
    )")

iEnd

iBegin(thread, ThreadID, "thread.id")
    iTarget(optype::int\ <64>)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());

    }

    iDoc(R"(    
        Returns the ID of the current virtual thread. Returns -1 if executed
        in the primary thread.
    )")

iEnd

iBegin(thread, ThreadSchedule, "thread.schedule")
    iOp1(optype::Function, trueX)
    iOp2(optype::tuple, trueX)
    iOp3(optype::[int\ <64>], trueX)

    iValidate {
        auto ty_op1 = as<type::Function>(op1->type());
        auto ty_op2 = as<type::tuple>(op2->type());
        auto ty_op3 = as<type::[int\ <64>]>(op3->type());

    }

    iDoc(R"(    
        Schedules a function call onto a virtual thread. If *op3* is not
        given, the current thread context determines the target thread,
        according to HILTI context-based scheduling. If *op3* is given, it
        gives the target thread ID directly; in this case the functions thread
        context will be cleared when running.
    )")

iEnd

iBegin(thread, SetContext, "thread.set_context")
    iOp1(optype::<HILTI Type>, trueX)

    iValidate {
        auto ty_op1 = as<type::<HILTI Type>>(op1->type());

    }

    iDoc(R"(    
        Sets the thread context of the currently executing virtual thread to
        *op1*. The type of *op1* must match the current module's ``context``
        definition.
    )")

iEnd

