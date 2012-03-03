///
/// \type X
/// 
/// \ctor X
///
/// \cproto X
///

iBegin(callable, New, "new")
    iTarget(optype::refCallable)
    iOp1(optype::typeCallable, true);

    iValidate {
    }

    iDoc(R"(
        Instantiates a new *callable* instance. The instance will not yet be bound to anything.
    )")

iEnd

iBegin(callable, Bind, "callable.bind")
    iTarget(optype::refCallable)
    iOp1(optype::function, true)
    iOp2(optype::tuple, true)

    iValidate {
    }

    iDoc(R"(    
        Binds arguments *op2* to a call of function *op1* and return the
        resulting callable.
    )")

iEnd

