iBegin(call, Call, "call")
    iTarget(optype::[type])
    iOp1(optype::function, trueX)
    iOp2(optype::(arguments), trueX)

    iValidate {
        auto ty_target = as<type::[type]>(target->type());
        auto ty_op1 = as<type::function>(op1->type());
        auto ty_op2 = as<type::(arguments)>(op2->type());

    }

    iDoc(R"(    
        Calls *function* using the tuple in *op2* as arguments. The argument
        types as well as the target's type must match the function's
        signature.
    )")

iEnd

iBegin(call, CallC, "call.c")
    iTarget(optype::[type])
    iOp1(optype::Function, trueX)
    iOp2(optype::tuple, trueX)

    iValidate {
        auto ty_target = as<type::[type]>(target->type());
        auto ty_op1 = as<type::Function>(op1->type());
        auto ty_op2 = as<type::tuple>(op2->type());

    }

    iDoc(R"(    
        For internal use only.  Calls the external C function using the tuple
        in *op2* as its arguments. The argument types as well as the target's
        type must match the function's signature.
    )")

iEnd

iBegin(call, CallTailResult, "call.tail.result")
    iTarget(optype::[type])
    iOp1(optype::function, trueX)
    iOp2(optype::XXX, trueX)
    iOp3(optype::XXX, trueX)

    iValidate {
        auto ty_target = as<type::[type]>(target->type());
        auto ty_op1 = as<type::function>(op1->type());
        auto ty_op2 = as<type::XXX>(op2->type());
        auto ty_op3 = as<type::XXX>(op3->type());

    }

    iDoc(R"(    
        For internal use only.  Like *Call()*, calls *function* using the
        tuple in *op2* as arguments. The argument types must match the
        function's signature. The function must return a result value and it's
        type must likewise match the function's signature. Different than
        *Call()*, *CallTailVoid* must be the *last instruction* of a *Block*.
        After the called function returns, control is passed to block *op3*.
        This instruction is for internal use only. During the code generation,
        all *Call* instructions are converted into either *CallTailVoid* or
        *CallTailResult* instructions.
    )")

iEnd

iBegin(call, CallTailVoid, "call.tail.void")
    iOp1(optype::function, trueX)
    iOp2(optype::XXX, trueX)
    iOp3(optype::XXX, trueX)

    iValidate {
        auto ty_op1 = as<type::function>(op1->type());
        auto ty_op2 = as<type::XXX>(op2->type());
        auto ty_op3 = as<type::XXX>(op3->type());

    }

    iDoc(R"(    
        For internal use only.  Calls *function* using the tuple in *op2* as
        arguments. The argument types must match the function's signature, and
        the function must not return a value. Different than *Call()*,
        *CallTailVoid* must be the *last instruction* of a *Block*. After the
        called function returns, control is passed to block *op3*.  This
        instruction is for internal use only. During the code generation, all
        *Call* instructions are converted into either *CallTailVoid* or
        *CallTailResult* instructions.
    )")

iEnd

