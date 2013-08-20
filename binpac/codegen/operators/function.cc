
#include "cg-operator-common.h"
#include <binpac/autogen/operators/function.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(expression::operator_::function::Call* i)
{
    auto func = ast::checkedCast<expression::Function>(i->op1());
    auto args = callParameters(i->op2());

    shared_ptr<hilti::Expression> cookie = nullptr;

    if ( in<declaration::Function>() || in<declaration::Hook>() || in<declaration::Type>() )
        cookie = hilti::builder::id::create("__cookie", i->location());
    else
        cookie = hilti::builder::reference::createNull(i->location());

    auto result = cg()->hiltiCall(func, args, cookie);
    setResult(result);
}
