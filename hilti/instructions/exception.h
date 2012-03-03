///
/// \type Exceptions.
///

iBegin(exception, New, "new")
    iTarget(optype::refException)
    iOp1(optype::typeException, true);

    iValidate {
    }

    iDoc(R"(
        Instantiates a new exception.
    )")

iEnd

iBegin(exception, NewWithArg, "new")
    iTarget(optype::refException)
    iOp1(optype::typeException, true);
    iOp2(optype::any, true);

    iValidate {
    }

    iDoc(R"(
        Instantiates a new exception, with *op1* as its argument.
    )")

iEnd

iBegin(exception, Throw, "exception.throw")
    iOp1(optype::refException, true)

    iValidate {
    }

    iDoc(R"(    
        Throws an exception, diverting control-flow up the stack to the
        closest handler.
    )")

iEnd

