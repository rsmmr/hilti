iBegin(decr_by, DecrBy, "decr_by")
    iTarget(optype::<HILTI Type>)
    iOp1(optype::<HILTI Type>, trueX)
    iOp2(optype::int\ <64>, trueX)

    iValidate {
        auto ty_target = as<type::<HILTI Type>>(target->type());
        auto ty_op1 = as<type::<HILTI Type>>(op1->type());
        auto ty_op2 = as<type::int\ <64>>(op2->type());

    }

    iDoc(R"(    
        Decrements *op1* by *op2* and returns the result. *op1* is not
        modified. Note that not all types supporting *incr* also support
        *incr_by*.
    )")

iEnd

