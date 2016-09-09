
#include "cg-operator-common.h"
#include <binpac/autogen/operators/set.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(ctor::Set* s)
{
    auto stype = ast::rtti::checkedCast<type::Set>(s->type());
    auto etype = cg()->hiltiType(stype->elementType());

    hilti::builder::set::element_list elems;

    for ( auto e : s->elements() )
        elems.push_back(cg()->hiltiExpression(e));

    auto result = hilti::builder::set::create(etype, elems, s->location());
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::set::In* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    auto result = cg()->builder()->addTmp("exists", hilti::builder::boolean::type());
    cg()->builder()->addInstruction(result, hilti::instruction::set::Exists, op2, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::set::Add* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(nullptr, hilti::instruction::set::Insert, op1, op2);
    setResult(op1);
}

void CodeBuilder::visit(expression::operator_::set::Delete* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(hilti::instruction::set::Remove, op1, op2);
    setResult(op1);
}

void CodeBuilder::visit(expression::operator_::set::Size* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto result = cg()->builder()->addTmp("size", cg()->hiltiType(i->type()));
    cg()->builder()->addInstruction(result, hilti::instruction::set::Size, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::set::Clear* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(nullptr, hilti::instruction::set::Clear, op1);
    setResult(op1);
}
