iBegin(new, New, "new")
    iTarget(optype::ref\ <<heap type>>)
    iOp1(optype::<heap type>, trueX)
    iOp2(optype::[type], trueX)
    iOp3(optype::[type], trueX)

    iValidate {
        auto ty_target = as<type::ref\ <<heap type>>>(target->type());
        auto ty_op1 = as<type::<heap type>>(op1->type());
        auto ty_op2 = as<type::[type]>(op2->type());
        auto ty_op3 = as<type::[type]>(op3->type());

    }

    iDoc(R"(    
        Allocates a new instance of type *op1*.
    )")

iEnd

