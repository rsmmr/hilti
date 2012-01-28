iBegin(decr, Decr, "decr")
    iTarget(optype::<HILTI Type>)
    iOp1(optype::<HILTI Type>, trueX)

    iValidate {
        auto ty_target = as<type::<HILTI Type>>(target->type());
        auto ty_op1 = as<type::<HILTI Type>>(op1->type());

    }

    iDoc(R"(    
        Decrements *op1* by one and returns the result. *op1* is not modified.
    )")

iEnd

