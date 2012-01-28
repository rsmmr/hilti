iBegin(callable, Bind, "callable.bind")
    iOp1(optype::ref\ <callable>, trueX)
    iOp2(optype::Function, trueX)
    iOp3(optype::tuple, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <callable>>(op1->type());
        auto ty_op2 = as<type::Function>(op2->type());
        auto ty_op3 = as<type::tuple>(op3->type());

    }

    iDoc(R"(    
        Binds arguments *op2* to a call of function *op1* and return the
        resulting callable.
    )")

iEnd

