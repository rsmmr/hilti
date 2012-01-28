iBegin(exception, Throw, "exception.throw")
    iOp1(optype::ref\ <exception>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <exception>>(op1->type());

    }

    iDoc(R"(    
        Throws an exception, diverting control-flow up the stack to the
        closest handler.
    )")

iEnd

