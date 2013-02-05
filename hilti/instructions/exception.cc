
#include "instructions/define-instruction.h"

#include "exception.h"

iBeginCC(exception)
    iValidateCC(New) {
        auto type = ast::as<expression::Type>(op1)->typeValue();
        auto etype = ast::as<type::Exception>(type);

        if ( etype->argType() )
            error(type, ::util::fmt("exception takes an argument of type %s", etype->argType()->render().c_str()));
    }

    iDocCC(New, R"(
        Instantiates a new exception.
    )")
iEndCC

iBeginCC(exception)
    iValidateCC(NewWithArg) {
        auto type = ast::as<expression::Type>(op1)->typeValue();
        auto etype = ast::as<type::Exception>(type);

        if ( ! etype->argType() )
            error(type, "exception does not take an argument");
        else
            canCoerceTo(op2, etype->argType());
    }

    iDocCC(NewWithArg, R"(
        Instantiates a new exception)
    )")
iEndCC

iBeginCC(exception)
    iValidateCC(Throw) {
    }

    iDocCC(Throw, R"(    
        Throws an exception)
        closest handler.
    )")
iEndCC

iBeginCC(exception)
    iValidateCC(__BeginHandler) {
    }

    iDocCC(__BeginHandler, R"(
       Internal instruction beginning scope of an exception handler. Note that this instruction is used statically at compile-time, not exectued at run-time. It is thus subject to its static location, not control flow.
    )")
iEndCC

iBeginCC(exception)
    iValidateCC(__EndHandler) {
    }

    iDocCC(__EndHandler, R"(
       Internal instruction ending scope of most current exception handler. Note that this instruction is used statically at compile-time, not exectued at run-time. It is thus subject to its static location, not control flow.
    )")
iEndCC

iBeginCC(exception)
    iValidateCC(__GetAndClear) {
    }

    iDocCC(__GetAndClear, R"(
       Internal instruction returning the currently raised exception, clearing it.
    )")
iEndCC

iBeginCC(exception)
    iValidateCC(__Clear) {
    }

    iDocCC(__Clear, R"(
       Internal instruction clearing the currently raised exception.
    )")
iEndCC

// iBeginCC(exception)
//     iValidateCC(__Match) {
//     }
// 
//     iSuccessorsCC(__Match) {
//         return { op2, op3 };
//     }
// 
//     iDocCC(__Match, R"(
//        Internal instruction matching the currently raised exception againt a type. If it matches, the instruction
//        branches to *op1*, otherwise to *op2*. If the type is *any*, it matches any currently raised exception.
//     )")
// iEndCC
// 
// iBeginCC(exception)
//     iValidateCC(__Current) {
//     }
// 
//     iDocCC(__Current, R"(
//        Internal instruction returning the currently raised exception)
//     )")
// iEndCC
// 
// 
// iBeginCC(exception)
//     iValidateCC(__Reraise) {
//     }
// 
//     iSuccessorsCC(__Reraise) {
//         return std::set<shared_ptr<Expression>>();
//     }
// 
//     iDocCC(__Reraise, R"(
//        Internal instruction that reraises the current exception up the stack to the next caller.
//     )")
// iEndCC
// #endif
// 
