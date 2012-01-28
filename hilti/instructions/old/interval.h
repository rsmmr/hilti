iBegin(interval, Add, "interval.add")
    iTarget(optype::interval)
    iOp1(optype::interval, trueX)
    iOp2(optype::interval, trueX)

    iValidate {
        auto ty_target = as<type::interval>(target->type());
        auto ty_op1 = as<type::interval>(op1->type());
        auto ty_op2 = as<type::interval>(op2->type());

    }

    iDoc(R"(    
        Adds intervals *op1* and *op2*.
    )")

iEnd

iBegin(interval, AsDouble, "interval.as_double")
    iTarget(optype::double)
    iOp1(optype::interval, trueX)

    iValidate {
        auto ty_target = as<type::double>(target->type());
        auto ty_op1 = as<type::interval>(op1->type());

    }

    iDoc(R"(    
        Returns *op1* in seconds, rounded down to the nearest value that can
        be represented as a double.
    )")

iEnd

iBegin(interval, AsInt, "interval.as_int")
    iTarget(optype::int\ <64>)
    iOp1(optype::interval, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::interval>(op1->type());

    }

    iDoc(R"(    
        Returns *op1*' in seconds, rounded down to the nearest integer.
    )")

iEnd

iBegin(interval, Eq, "interval.eq")
    iTarget(optype::bool)
    iOp1(optype::interval, trueX)
    iOp2(optype::interval, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::interval>(op1->type());
        auto ty_op2 = as<type::interval>(op2->type());

    }

    iDoc(R"(    
        Returns True if the interval represented by *op1* equals that of
        *op2*.
    )")

iEnd

iBegin(interval, Gt, "interval.gt")
    iTarget(optype::bool)
    iOp1(optype::interval, trueX)
    iOp2(optype::interval, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::interval>(op1->type());
        auto ty_op2 = as<type::interval>(op2->type());

    }

    iDoc(R"(    
        Returns True if the interval represented by *op1* is larger than that
        of *op2*.
    )")

iEnd

iBegin(interval, Lt, "interval.lt")
    iTarget(optype::bool)
    iOp1(optype::interval, trueX)
    iOp2(optype::interval, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::interval>(op1->type());
        auto ty_op2 = as<type::interval>(op2->type());

    }

    iDoc(R"(    
        Returns True if the interval represented by *op1* is shorter than that
        of *op2*.
    )")

iEnd

iBegin(interval, Sub, "interval.mul")
    iTarget(optype::interval)
    iOp1(optype::interval, trueX)
    iOp2(optype::int\ <64>, trueX)

    iValidate {
        auto ty_target = as<type::interval>(target->type());
        auto ty_op1 = as<type::interval>(op1->type());
        auto ty_op2 = as<type::int\ <64>>(op2->type());

    }

    iDoc(R"(    
        Subtracts interval *op2* to the interval *op1*
    )")

iEnd

iBegin(interval, Nsecs, "interval.nsecs")
    iTarget(optype::int\ <64>)
    iOp1(optype::interval, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::interval>(op1->type());

    }

    iDoc(R"(    
        Returns the interval *op1* as nanoseconds since the epoch.
    )")

iEnd

iBegin(interval, Sub, "interval.sub")
    iTarget(optype::interval)
    iOp1(optype::interval, trueX)
    iOp2(optype::interval, trueX)

    iValidate {
        auto ty_target = as<type::interval>(target->type());
        auto ty_op1 = as<type::interval>(op1->type());
        auto ty_op2 = as<type::interval>(op2->type());

    }

    iDoc(R"(    
        Subtracts interval *op2* to the interval *op1*
    )")

iEnd

