iBegin(timer, Cancel, "timer.cancel")
    iOp1(optype::ref\ <timer>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <timer>>(op1->type());

    }

    iDoc(R"(    
        Cancels the timer *op1*. This also detaches the timer from its
        manager, and allows it to be rescheduled subsequently.
    )")

iEnd

iBegin(timer, Update, "timer.update")
    iOp1(optype::ref\ <timer>, trueX)
    iOp2(optype::time, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <timer>>(op1->type());
        auto ty_op2 = as<type::time>(op2->type());

    }

    iDoc(R"(    
        Adjusts *op1*'s expiration time to *op2*.  Raises
        ``TimerNotScheduled`` if the timer is not scheduled with a timer
        manager.
    )")

iEnd

