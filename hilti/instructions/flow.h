
// Instructions controlling flow.

iBegin(flow, ReturnResult, "return.result")
    iOp1(type::Any, true)

    iValidate {
    }

    iDoc("")
iEnd

iBegin(flow, ReturnVoid, "return.void")
    iValidate {
    }

    iDoc("")
iEnd

iBegin(flow, CallVoid, "call")
    iOp1(type::Function, true)
    iOp2(type::Tuple, true)

    iValidate {
    }

    iDoc("")
iEnd

iBegin(flow, CallResult, "call")
    iTarget(type::Any)
    iOp1(type::Function, true)
    iOp2(type::Tuple, true)

    iValidate {
    }

    iDoc("")
iEnd
