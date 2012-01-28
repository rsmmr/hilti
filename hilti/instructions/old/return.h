iBegin(return, ReturnResult, "return.result")
    iOp1(optype::[type], trueX)

    iValidate {
        auto ty_op1 = as<type::[type]>(op1->type());

    }

    iDoc(R"(    
        Returns from the current function with the given value. The type of
        the value must match the function's signature.
    )")

iEnd

iBegin(return, ReturnVoid, "return.void")

    iValidate {

    }

    iDoc(R"(    
        Returns from the current function without returning any value. The
        function's signature must not specifiy a return value.
    )")

iEnd

