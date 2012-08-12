
#include "coercion-builder.h"
#include "expression.h"
#include "type.h"

using namespace binpac;
using namespace binpac::codegen;

CoercionBuilder::CoercionBuilder(CodeGen* cg)
    : CGVisitor<shared_ptr<hilti::Expression>, shared_ptr<hilti::Expression>, shared_ptr<Type>>(cg, "CoercionBuilder")
{
}

CoercionBuilder::~CoercionBuilder()
{
}

shared_ptr<hilti::Expression> CoercionBuilder::hiltiCoerce(shared_ptr<hilti::Expression> expr, shared_ptr<Type> src, shared_ptr<Type> dst)
{
    if ( src->equal(dst) || dst->equal(src) )
        return expr;

    auto opt = ast::tryCast<type::OptionalArgument>(dst);

    if ( opt )
        return hiltiCoerce(expr, src, opt->argType());

    setArg1(expr);
    setArg2(dst);

    shared_ptr<hilti::Expression> result;
    bool success = processOne(src, &result);
    assert(success);

    return result;
}
