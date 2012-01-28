iBegin(unequal, Unequal, "unequal")
    iTarget(optype::bool)
    iOp1(optype::<value type>, trueX)
    iOp2(optype::<value type>, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::<value type>>(op1->type());
        auto ty_op2 = as<type::<value type>>(op2->type());

    }

    iDoc(R"(    
        Returns True if *op1* does not equal *op2*.  Note: This operator is
        automatically defined for all types that provide an ``equaul` operator
        by negating its result.
    )")

iEnd

