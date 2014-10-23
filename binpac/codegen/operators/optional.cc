
#include "cg-operator-common.h"
#include <binpac/autogen/operators/optional.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(constant::Optional* o)
{
    std::shared_ptr<::hilti::Expression> c;

    if ( o->value() )
        c = hilti::builder::union_::create(cg()->hiltiExpression(o->value()));
    else
        c = hilti::builder::union_::create();

    setResult(c);
}

void CodeBuilder::visit(expression::operator_::optional::CoerceOptional* i)
{
    auto otype = ast::checkedCast<type::Optional>(i->op1()->type());
    auto htype = cg()->hiltiType(otype->argType());

    auto result = builder()->addTmp("uval", htype);
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::union_::GetType, op1);

    setResult(::hilti::builder::union_::create(nullptr, result));
}

void CodeBuilder::visit(expression::operator_::optional::CoerceUnwrap* i)
{
    auto otype = ast::checkedCast<type::Optional>(i->op1()->type());

    assert(i->type());

    auto htype = cg()->hiltiType(i->type());
    auto result = builder()->addTmp("uval", htype);
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::union_::GetType, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::optional::CoerceWrap* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    setResult(::hilti::builder::union_::create(nullptr, op1));
}
