
#include "cg-operator-common.h"
#include <spicy/autogen/operators/list.h>

using namespace spicy;
using namespace spicy::codegen;

void CodeBuilder::visit(ctor::List* l)
{
    auto ltype = ast::rtti::checkedCast<type::List>(l->type());
    auto etype = cg()->hiltiType(ltype->elementType());

    hilti::builder::list::element_list elems;

    for ( auto e : l->elements() )
        elems.push_back(cg()->hiltiExpression(e));

    auto result = hilti::builder::list::create(etype, elems, l->location());
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::list::PlusAssign* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(hilti::instruction::list::Append, op1, op2);
    setResult(op1);
}

void CodeBuilder::visit(expression::operator_::list::PushBack* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto elem = cg()->hiltiExpression(callParameter(i->op3(), 0));
    cg()->builder()->addInstruction(hilti::instruction::list::PushBack, op1, elem);
    setResult(op1);
}

void CodeBuilder::visit(expression::operator_::list::Size* i)
{
    auto result = builder()->addTmp("size", cg()->hiltiType(i->type()));
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::list::Size, op1);
    setResult(result);
}
