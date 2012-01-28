iBegin(ref, CastBool, "ref.cast.bool")
    iTarget(optype::bool)
    iOp1(optype::ref, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::ref>(op1->type());

    }

    iDoc(R"(    
        Converts *op1* into a boolean. The boolean's value will be True if
        *op1* references a valid object, and False otherwise.
    )")

iEnd

