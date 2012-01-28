iBegin(equal, Equal, "equal")
    iTarget(optype::bool)
    iOp1(optype::<HILTI Type>, trueX)
    iOp2(optype::<HILTI Type>, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::<HILTI Type>>(op1->type());
        auto ty_op2 = as<type::<HILTI Type>>(op2->type());

    }

    iDoc(R"(    
        Returns True if *op1* equals *op2*
    )")

iEnd

