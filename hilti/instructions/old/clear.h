iBegin(clear, Clear, "clear")
    iOp1(optype::<value type>, trueX)

    iValidate {
        auto ty_op1 = as<type::<value type>>(op1->type());

    }

    iDoc(R"(    
        Resets *op1* to the default value a new variable would be set to.
        Note: This operator is automatically defined for all value types.
    )")

iEnd

