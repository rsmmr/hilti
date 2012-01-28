iBegin(jump, Jump, "jump")
    iOp1(optype::Label, trueX)

    iValidate {
        auto ty_op1 = as<type::Label>(op1->type());

    }

    iDoc(R"(    
        Jumps unconditionally to label *op2*.
    )")

iEnd

