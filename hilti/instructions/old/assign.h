iBegin(assign, Assign, "assign")
    iTarget(optype::<value type>)
    iOp1(optype::<value type>, trueX)

    iValidate {
        auto ty_target = as<type::<value type>>(target->type());
        auto ty_op1 = as<type::<value type>>(op1->type());

    }

    iDoc(R"(    
        Assigns *op1* to the target.  There is a short-cut syntax: instead of
        using the standard form ``t = assign op``, one can just write ``t =
        op``.  Note: The ``assign`` operator uses a generic implementation
        able to handle all data types. Different from most other operators,
        it's implementation is not overloaded on a per-type based.
    )")

iEnd

