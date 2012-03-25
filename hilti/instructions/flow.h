
// Instructions controlling flow.

iBegin(flow, ReturnResult, "return.result")
    iTerminator()
    iOp1(optype::any, true)

    iValidate {
        auto hook = validator()->current<declaration::Hook>();

        if ( hook ) {
            error(nullptr, "return.result must not be used inside a hook; use hook.stop instead");
            return;
        }
    }

    iDoc("")
iEnd

iBegin(flow, ReturnVoid, "return.void")
    iTerminator()
    iValidate {
    }

    iDoc("")
iEnd

iBegin(flow, CallVoid, "call")
    iOp1(optype::function, true)
    iOp2(optype::tuple, true)

    iValidate {
    }

    iDoc("")
iEnd

iBegin(flow, CallResult, "call")
    iTarget(optype::any)
    iOp1(optype::function, true)
    iOp2(optype::tuple, true)

    iValidate {
    }

    iDoc("")
iEnd

iBegin(flow, IfElse, "if.else")
    iTerminator()
    iOp1(optype::boolean, true)
    iOp2(optype::label, true)
    iOp3(optype::label, true)

    iValidate {
    }

    iDoc(R"(    
        Transfers control label *op2* if *op1* is true, and to *op3*
        otherwise.
    )")

iEnd

iBegin(flow, Jump, "jump")
    iTerminator()
    iOp1(optype::label, true)

    iValidate {
    }

    iDoc(R"(    
        Jumps unconditionally to label *op2*.
    )")

iEnd

