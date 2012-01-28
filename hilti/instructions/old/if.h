iBegin(if, IfElse, "if.else")
    iOp1(optype::bool, trueX)
    iOp2(optype::Label, trueX)
    iOp3(optype::Label, trueX)

    iValidate {
        auto ty_op1 = as<type::bool>(op1->type());
        auto ty_op2 = as<type::Label>(op2->type());
        auto ty_op3 = as<type::Label>(op3->type());

    }

    iDoc(R"(    
        Transfers control label *op2* if *op1* is true, and to *op3*
        otherwise.
    )")

iEnd

