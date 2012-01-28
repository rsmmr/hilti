iBegin(enum, FromInt, "enum.from_int")
    iTarget(optype::enum)
    iOp1(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::enum>(target->type());
        auto ty_op1 = as<type::int>(op1->type());

    }

    iDoc(R"(    
        Converts the integer *op2* into an enum of the target's *type*. If the
        integer corresponds to a valid label (as set by explicit
        initialization), the results corresponds to that label. If integer
        does not correspond to any lable, the result will be of value
        ``Undef`` (yet internally, the integer value is retained.)
    )")

iEnd

