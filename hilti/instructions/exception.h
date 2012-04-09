///
/// \type Exceptions.
///

#include "instructions/define-instruction.h"

iBegin(exception, New, "new")
    iTarget(optype::refException)
    iOp1(optype::typeException, true);

    iValidate {
        auto type = ast::as<expression::Type>(op1)->typeValue();
        auto etype = ast::as<type::Exception>(type);

        if ( etype->argType() )
            error(type, ::util::fmt("exception takes an argument of type %s", etype->argType()->render().c_str()));
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
        auto type = ast::as<expression::Type>(op1)->typeValue();
        auto etype = ast::as<type::Exception>(type);

        if ( ! etype->argType() )
            error(type, "exception does not take an argument");
        else
            canCoerceTo(op2, etype->argType());
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

