iBegin(select, Select, "select")
    iTarget(optype::type)
    iOp1(optype::bool, trueX)
    iOp2(optype::type, trueX)
    iOp3(optype::type, trueX)

    iValidate {
        auto ty_target = as<type::type>(target->type());
        auto ty_op1 = as<type::bool>(op1->type());
        auto ty_op2 = as<type::type>(op2->type());
        auto ty_op3 = as<type::type>(op3->type());

    }

    iDoc(R"(    
        Returns *op2* if *op1* is True, and *op3* otherwise.
    )")

iEnd

