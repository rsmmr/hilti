
#include "cg-operator-common.h"
#include <spicy/autogen/operators/vector.h>

using namespace spicy;
using namespace spicy::codegen;

void CodeBuilder::visit(ctor::Vector* v)
{
    auto vtype = ast::rtti::checkedCast<type::Vector>(v->type());
    auto etype = cg()->hiltiType(vtype->elementType());

    hilti::builder::list::element_list elems;

    for ( auto e : v->elements() )
        elems.push_back(cg()->hiltiExpression(e));

    auto result = hilti::builder::vector::create(etype, elems, v->location());
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::vector::Index* i)
{
    auto ctype = ast::type::checkedTrait<type::trait::Container>(i->op1()->type());
    auto etype = cg()->hiltiType(ctype->elementType());

    auto result = builder()->addTmp("elem", etype);
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::vector::Get, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::vector::IndexAssign* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    auto op3 = cg()->hiltiExpression(i->op3());
    cg()->builder()->addInstruction(hilti::instruction::vector::Set, op1, op2, op3);
}

void CodeBuilder::visit(expression::operator_::vector::PushBack* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto elem = cg()->hiltiExpression(callParameter(i->op3(), 0));
    cg()->builder()->addInstruction(hilti::instruction::vector::PushBack, op1, elem);
    setResult(op1);
}


void CodeBuilder::visit(expression::operator_::vector::Reserve* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto size = cg()->hiltiExpression(callParameter(i->op3(), 0));
    cg()->builder()->addInstruction(hilti::instruction::vector::Reserve, op1, size);
    setResult(op1);
}

void CodeBuilder::visit(expression::operator_::vector::Size* i)
{
    auto result = builder()->addTmp("size", cg()->hiltiType(i->type()));
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::vector::Size, op1);
    setResult(result);
}
