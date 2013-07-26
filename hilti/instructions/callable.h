///
/// \type X
/// 
/// \ctor X
///
/// \cproto X
///

#include "define-instruction.h"

iBegin(callable, New, "new")
    iTarget(optype::refCallable)
    iOp1(optype::typeCallable, true);
    iOp2(optype::function, true)
    iOp3(optype::tuple, true)

    iValidate {
        auto ctype = ast::as<type::Callable>(typedType(op1));
        auto ftype = ast::as<type::Function>(op2->type());
        equalTypes(ctype->argType(), ftype->result()->type());
    }

    iDoc(R"(
        Instantiates a new *callable* instance, binding
        arguments *op2* to a call of function *op1*.
    )")

iEnd

