
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

void CoercionBuilder::visit(type::Integer* i)
{
    auto val = arg1();
    auto dst = arg2();

    shared_ptr<type::Bool> dst_b = ast::as<type::Bool>(dst);

    if ( dst_b ) {
        setResult(val); // HILTI coerces this.
        return;
    }

    auto dst_i = ast::as<type::Integer>(dst);

    if ( dst_i ) {
        auto tmp = cg()->builder()->addTmp("ext_int", cg()->hiltiType(dst_i));

        if ( dst_i->signed_() )
            cg()->builder()->addInstruction(tmp, hilti::instruction::integer::SExt, val);
        else
            cg()->builder()->addInstruction(tmp, hilti::instruction::integer::ZExt, val);

        setResult(tmp);
        return;
    }

    assert(false);
}
