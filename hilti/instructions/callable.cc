
#include "define-instruction.h"

#include "callable.h"
#include "../module.h"

static void _checkBinding(const Instruction* i, shared_ptr<Expression> op1, shared_ptr<Expression> op2, shared_ptr<Expression> op3)
{
    // Check parameters against function being bound.
    auto ftype = as<type::Function>(op2->type());

    if ( ! i->checkCallParameters(ftype, op3, true) )
        return;

    // Check that function's return type matches callable.
    auto ctype = ast::checkedCast<type::Callable>(i->typedType(op1));

    if ( ! i->checkCallResult(ctype->result()->type(), ftype->result()->type()) )
        return;

    // Check that unbound parameters match callable.
    auto ttype = ast::checkedCast<type::Tuple>(op3->type());
    auto cparams = ctype->Function::parameters();
    auto fparams = ftype->parameters();

    for ( int i = 0; i < ttype->typeList().size(); i++ )
        fparams.pop_front();

    if ( cparams.size() != fparams.size() ) {
        i->error(nullptr, util::fmt("number of unbound arguments do not match callable (expected %u, have %u)",
                                 cparams.size(), fparams.size()));
        return;
    }

    for ( auto p : util::zip2(cparams, fparams) )
        i->equalTypes(p.first->type(), p.second->type());
}

iBeginCC(callable)
    iValidateCC(NewFunction) {
        return _checkBinding(this, op1, op2, op3);
    }

    iDocCC(NewFunction ,R"(
        Instantiates a new *callable* instance, binding arguments *op2* to a call of function *op1*. If *op2* has less element than the function expects, the callable will be bound partially.
    )")
iEndCC

iBeginCC(callable)
    iValidateCC(NewHook) {
        return _checkBinding(this, op1, op2, op3);
    }

    iDocCC(NewHook ,R"(
        Instantiates a new *callable* instance, binding arguments *op2* to an execution of hook *op1*. If *op2* has less element than the hook expects, the callable will be bound partially.
    )")
iEndCC


