
#include "cg-operator-common.h"
#include <binpac/autogen/operators/time.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(constant::Time* t)
{
    auto c = hilti::builder::time::create(t->value(), true);
    setResult(c);
}

void CodeBuilder::visit(expression::operator_::time::CastDouble* i)
{
    auto result = builder()->addTmp("d", hilti::builder::double_::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::time::AsDouble, op1);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::time::CastInteger* i)
{
    auto result = builder()->addTmp("i", hilti::builder::integer::type(64));
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::time::AsInt, op1);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::time::CoerceBool* i)
{
    auto result = builder()->addTmp("notnull", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto null = hilti::builder::time::create((uint64_t)0);
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Unequal, op1, null);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::time::Equal* i)
{
    auto result = builder()->addTmp("equal", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Equal, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::time::Greater* i)
{
    auto result = builder()->addTmp("gt", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::time::Gt, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::time::Lower* i)
{
    auto result = builder()->addTmp("lt", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::time::Lt, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::time::Nsecs* i)
{
    auto result = builder()->addTmp("ns", hilti::builder::integer::type(64));
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::time::Nsecs, op1);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::time::PlusInterval1* i)
{
    auto result = builder()->addTmp("sum", hilti::builder::time::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::time::Add, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::time::PlusInterval2* i)
{
    auto result = builder()->addTmp("sum", hilti::builder::time::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::time::Add, op2, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::time::Minus* i)
{
    auto result = builder()->addTmp("sub", hilti::builder::interval::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::time::SubTime, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::time::MinusInterval* i)
{
    auto result = builder()->addTmp("sub", hilti::builder::time::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::time::SubInterval, op1, op2);
    setResult(result);
}

