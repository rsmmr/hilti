iBegin(bool, And, "bool.and")
    iTarget(optype::bool)
    iOp1(optype::bool, trueX)
    iOp2(optype::bool, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::bool>(op1->type());
        auto ty_op2 = as<type::bool>(op2->type());

    }

    iDoc(R"(    
        Computes the logical 'and' of *op1* and *op2*.
    )")

iEnd

iBegin(bool, Not, "bool.not")
    iTarget(optype::bool)
    iOp1(optype::bool, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::bool>(op1->type());

    }

    iDoc(R"(    
        Computes the logical 'not' of *op1*.
    )")

iEnd

iBegin(bool, Or, "bool.or")
    iTarget(optype::bool)
    iOp1(optype::bool, trueX)
    iOp2(optype::bool, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::bool>(op1->type());
        auto ty_op2 = as<type::bool>(op2->type());

    }

    iDoc(R"(    
        Computes the logical 'or' of *op1* and *op2*.
    )")

iEnd

