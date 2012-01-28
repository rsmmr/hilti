iBegin(tuple, Index, "tuple.index")
    iTarget(optype::any)
    iOp1(optype::tuple, trueX)
    iOp2(optype::any, trueX)

    iValidate {
        auto ty_target = as<type::any>(target->type());
        auto ty_op1 = as<type::tuple>(op1->type());
        auto ty_op2 = as<type::any>(op2->type());

    }

    iDoc(R"(    
        Returns the tuple's value with index *op2*. The index is zero-based.
        *op2* must be an integer *constant*.
    )")

iEnd

