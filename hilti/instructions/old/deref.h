iBegin(deref, Deref, "deref")
    iTarget(optype::<HILTI Type>)
    iOp1(optype::<HILTI Type>, trueX)

    iValidate {
        auto ty_target = as<type::<HILTI Type>>(target->type());
        auto ty_op1 = as<type::<HILTI Type>>(op1->type());

    }

    iDoc(R"(    
        Dereferences *op1* and returns the result.
    )")

iEnd

