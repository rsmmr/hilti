iBegin(, NextCallable, ".callable.next")
    iTarget(optype::ref\ <callable>)

    iValidate {
        auto ty_target = as<type::ref\ <callable>>(target->type());

    }

    iDoc(R"(    
        Internal instruction. Retrieves the next not-yet-processed callable
        registered by the C run-time.  Note: The returned reference will
        remain valid only until either the next time this instruction or any
        other instruction potentially registering new callables is called.
    )")

iEnd

iBegin(, PopHandler, ".pop_exception_handler")

    iValidate {

    }

    iDoc(R"(    
        Uninstalls the most recently installed exception handler. Exceptions
        of its type are no longer caught in subsequent code.
    )")

iEnd

iBegin(, PushHandler, ".push_exception_handler")
    iOp1(optype::Label, trueX)
    iOp2(optype::[exception], trueX)

    iValidate {
        auto ty_op1 = as<type::Label>(op1->type());
        auto ty_op2 = as<type::[exception]>(op2->type());

    }

    iDoc(R"(    
        Installs an exception handler for exceptions of *op1*, including any
        derived from that type. If such an exception is thrown subsequently,
        control is transfered to *op2*.
    )")

iEnd

