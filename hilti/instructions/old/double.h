iBegin(double, Add, "double.add")
    iTarget(optype::double)
    iOp1(optype::double, trueX)
    iOp2(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::double>(target->type());
        auto ty_op1 = as<type::double>(op1->type());
        auto ty_op2 = as<type::double>(op2->type());

    }

    iDoc(R"(    
        Calculates the sum of the two operands. If the sum overflows the range
        of the double type, the result in undefined.
    )")

iEnd

iBegin(double, AsInterval, "double.as_interval")
    iTarget(optype::interval)
    iOp1(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::interval>(target->type());
        auto ty_op1 = as<type::double>(op1->type());

    }

    iDoc(R"(    
        Converts the double *op1* into an interval value, interpreting it as
        seconds.
    )")

iEnd

iBegin(double, AsUint, "double.as_sint")
    iTarget(optype::int)
    iOp1(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::double>(op1->type());

    }

    iDoc(R"(    
        Converts the double *op1* into a signed integer value, rounding to the
        nearest value.
    )")

iEnd

iBegin(double, AsTime, "double.as_time")
    iTarget(optype::time)
    iOp1(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::time>(target->type());
        auto ty_op1 = as<type::double>(op1->type());

    }

    iDoc(R"(    
        Converts the double *op1* into a time value, interpreting it as
        seconds since the epoch.
    )")

iEnd

iBegin(double, AsUint, "double.as_uint")
    iTarget(optype::int)
    iOp1(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::double>(op1->type());

    }

    iDoc(R"(    
        Converts the double *op1* into an unsigned integer value, rounding to
        the nearest value.
    )")

iEnd

iBegin(double, Div, "double.div")
    iTarget(optype::double)
    iOp1(optype::double, trueX)
    iOp2(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::double>(target->type());
        auto ty_op1 = as<type::double>(op1->type());
        auto ty_op2 = as<type::double>(op2->type());

    }

    iDoc(R"(    
        Divides *op1* by *op2*, flooring the result. Throws
        :exc:`DivisionByZero` if *op2* is zero.
    )")

iEnd

iBegin(double, Eq, "double.eq")
    iTarget(optype::bool)
    iOp1(optype::double, trueX)
    iOp2(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::double>(op1->type());
        auto ty_op2 = as<type::double>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* equals *op2*.
    )")

iEnd

iBegin(double, Gt, "double.gt")
    iTarget(optype::bool)
    iOp1(optype::double, trueX)
    iOp2(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::double>(op1->type());
        auto ty_op2 = as<type::double>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* is greater than *op2*.
    )")

iEnd

iBegin(double, Lt, "double.lt")
    iTarget(optype::bool)
    iOp1(optype::double, trueX)
    iOp2(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::double>(op1->type());
        auto ty_op2 = as<type::double>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* is less than *op2*.
    )")

iEnd

iBegin(double, Mod, "double.mod")
    iTarget(optype::double)
    iOp1(optype::double, trueX)
    iOp2(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::double>(target->type());
        auto ty_op1 = as<type::double>(op1->type());
        auto ty_op2 = as<type::double>(op2->type());

    }

    iDoc(R"(    
        Calculates *op1* modulo *op2*. Throws :exc:`DivisionByZero` if *op2*
        is zero.
    )")

iEnd

iBegin(double, Mul, "double.mul")
    iTarget(optype::double)
    iOp1(optype::double, trueX)
    iOp2(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::double>(target->type());
        auto ty_op1 = as<type::double>(op1->type());
        auto ty_op2 = as<type::double>(op2->type());

    }

    iDoc(R"(    
        Multiplies *op1* with *op2*. If the product overflows the range of the
        double type, the result in undefined.
    )")

iEnd

iBegin(double, Pow, "double.pow")
    iTarget(optype::double)
    iOp1(optype::double, trueX)
    iOp2(optype::int<32> | double, trueX)

    iValidate {
        auto ty_target = as<type::double>(target->type());
        auto ty_op1 = as<type::double>(op1->type());
        auto ty_op2 = as<type::int<32> | double>(op2->type());

    }

    iDoc(R"(    
        Raises *op1* to the power *op2*. If the product overflows the range of
        the double type, the result in undefined.
    )")

iEnd

iBegin(double, Sub, "double.sub")
    iTarget(optype::double)
    iOp1(optype::double, trueX)
    iOp2(optype::double, trueX)

    iValidate {
        auto ty_target = as<type::double>(target->type());
        auto ty_op1 = as<type::double>(op1->type());
        auto ty_op2 = as<type::double>(op2->type());

    }

    iDoc(R"(    
        Subtracts *op1* one from *op2*. If the difference underflows the range
        of the double type, the result in undefined.
    )")

iEnd

