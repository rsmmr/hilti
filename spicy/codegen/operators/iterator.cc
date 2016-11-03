
#include "cg-operator-common.h"
#include <spicy/autogen/operators/iterator.h>

using namespace spicy;
using namespace spicy::codegen;

void CodeBuilder::visit(expression::operator_::iterator::Equal* i)
{
    auto result = builder()->addTmp("equal", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Equal, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::iterator::Deref* i)
{
    auto itype = ast::rtti::checkedCast<type::Iterator>(i->op1()->type());
    auto etype = cg()->hiltiType(
        ast::type::checkedTrait<type::trait::Iterable>(itype->argType())->elementType());

    auto result = builder()->addTmp("elem", etype);
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Deref, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::iterator::IncrPostfix* i)
{
    auto itype = cg()->hiltiType(i->op1()->type());
    auto result = builder()->addTmp("iter", itype);
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Assign, op1);
    cg()->builder()->addInstruction(op1, hilti::instruction::operator_::Incr, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::iterator::IncrPrefix* i)
{
    auto itype = cg()->hiltiType(i->op1()->type());
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(op1, hilti::instruction::operator_::Incr, op1);
    setResult(op1);
}

// We don't have a generic incrby hilti operator yet.

void CodeBuilder::visit(expression::operator_::iterator::Plus* i)
{
    auto itype = cg()->hiltiType(i->op1()->type());
    auto result = builder()->addTmp("iter", itype);
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2(), std::make_shared<type::Integer>(64, false));
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Assign, op1);
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::IncrBy, result, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::iterator::PlusAssign* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());

    cg()->builder()->addInstruction(op1, hilti::instruction::operator_::IncrBy, op1, op2);
    setResult(op1);
}
