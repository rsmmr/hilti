
// Instructions controlling flow.

iBegin(flow, ReturnResult, "return.result")
    iOp1(optype::any, true)

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
