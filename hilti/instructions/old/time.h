iBegin(time, Add, "time.add")
    iTarget(optype::time)
    iOp1(optype::time, trueX)
    iOp2(optype::interval, trueX)

    iValidate {
        auto ty_target = as<type::time>(target->type());
        auto ty_op1 = as<type::time>(op1->type());
        auto ty_op2 = as<type::interval>(op2->type());

    }

    iDoc(R"(    
        Adds interval *op2* to the time *op1*
    )")

iEnd

iBegin(time, AsDouble, "time.as_double")
    iTarget(optype::double)
    iOp1(optype::time, trueX)

    iValidate {
        auto ty_target = as<type::double>(target->type());
        auto ty_op1 = as<type::time>(op1->type());

    }

    iDoc(R"(    
        Returns *op1* in seconds since the epoch, rounded down to the nearest
        value that can be represented as a double.
    )")

iEnd

iBegin(time, AsInt, "time.as_int")
    iTarget(optype::int\ <64>)
    iOp1(optype::time, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::time>(op1->type());

    }

    iDoc(R"(    
        Returns *op1*' in seconds since the epoch, rounded down to the nearest
        integer.
    )")

iEnd

iBegin(time, Eq, "time.eq")
    iTarget(optype::bool)
    iOp1(optype::time, trueX)
    iOp2(optype::time, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::time>(op1->type());
        auto ty_op2 = as<type::time>(op2->type());

    }

    iDoc(R"(    
        Returns True if the time represented by *op1* equals that of *op2*.
    )")

iEnd

iBegin(time, Gt, "time.gt")
    iTarget(optype::bool)
    iOp1(optype::time, trueX)
    iOp2(optype::time, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::time>(op1->type());
        auto ty_op2 = as<type::time>(op2->type());

    }

    iDoc(R"(    
        Returns True if the time represented by *op1* is later than that of
        *op2*.
    )")

iEnd

iBegin(time, Lt, "time.lt")
    iTarget(optype::bool)
    iOp1(optype::time, trueX)
    iOp2(optype::time, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::time>(op1->type());
        auto ty_op2 = as<type::time>(op2->type());

    }

    iDoc(R"(    
        Returns True if the time represented by *op1* is earlier than that of
        *op2*.
    )")

iEnd

iBegin(time, Nsecs, "time.nsecs")
    iTarget(optype::int\ <64>)
    iOp1(optype::time, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::time>(op1->type());

    }

    iDoc(R"(    
        Returns the time *op1* as nanoseconds since the epoch.
    )")

iEnd

iBegin(time, Sub, "time.sub")
    iTarget(optype::time)
    iOp1(optype::time, trueX)
    iOp2(optype::interval, trueX)

    iValidate {
        auto ty_target = as<type::time>(target->type());
        auto ty_op1 = as<type::time>(op1->type());
        auto ty_op2 = as<type::interval>(op2->type());

    }

    iDoc(R"(    
        Subtracts interval *op2* to the time *op1*
    )")

iEnd

iBegin(time, Wall, "time.wall")
    iTarget(optype::time)

    iValidate {
        auto ty_target = as<type::time>(target->type());

    }

    iDoc(R"(    
        Returns the current wall time (i.e., real-time). Note that the
        resolution of the returned value is OS dependent.
    )")

iEnd

