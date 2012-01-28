iBegin(yield, Yield, "yield")

    iValidate {

    }

    iDoc(R"(    
        Yields processing back to the current scheduler, to be resumed later.
        If running in a virtual thread other than zero, this instruction yield
        to other virtual threads running within the same physical thread. Of
        running in virtual thread zero (or in non-threading mode), returns
        execution back to the calling C function (see interfacing with C).
    )")

iEnd

iBegin(yield, YieldUntil, "yield.until")
    iOp1(optype::ref\ <blockable>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <blockable>>(op1->type());

    }

    iDoc(R"(    
        XXX
    )")

iEnd

