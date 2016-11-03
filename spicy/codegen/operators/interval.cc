
#include "cg-operator-common.h"
#include <spicy/autogen/operators/interval.h>

using namespace spicy;
using namespace spicy::codegen;

void CodeBuilder::visit(constant::Interval* i)
{
    auto c = hilti::builder::interval::create(i->value(), true);
    setResult(c);
}

void CodeBuilder::visit(expression::operator_::interval::CastDouble* i)
{
    auto result = builder()->addTmp("d", hilti::builder::double_::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::interval::AsDouble, op1);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::interval::CastInteger* i)
{
    auto result = builder()->addTmp("i", cg()->hiltiType(i->type()));
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::interval::AsInt, op1);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::interval::CoerceBool* i)
{
    auto result = builder()->addTmp("notnull", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto null = hilti::builder::interval::create((uint64_t)0);
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Unequal, op1, null);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::interval::Equal* i)
{
    auto result = builder()->addTmp("equal", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Equal, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::interval::Greater* i)
{
    auto result = builder()->addTmp("gt", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::interval::Gt, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::interval::Lower* i)
{
    auto result = builder()->addTmp("lt", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::interval::Lt, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::interval::MultInt1* i)
{
    auto result = builder()->addTmp("prod", hilti::builder::interval::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::interval::Mul, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::interval::MultInt2* i)
{
    auto result = builder()->addTmp("prod", hilti::builder::interval::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::interval::Mul, op2, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::interval::Nsecs* i)
{
    auto result = builder()->addTmp("ns", cg()->hiltiType(i->type()));
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::interval::Nsecs, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::interval::Plus* i)
{
    auto result = builder()->addTmp("sum", hilti::builder::interval::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::interval::Add, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::interval::Minus* i)
{
    auto result = builder()->addTmp("diff", hilti::builder::interval::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::interval::Sub, op1, op2);
    setResult(result);
}
