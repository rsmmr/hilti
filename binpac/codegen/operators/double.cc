
#include "cg-operator-common.h"
#include <binpac/autogen/operators/double.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(constant::Double* d)
{
    auto c = hilti::builder::double_::create(d->value());
    setResult(c);
}

void CodeBuilder::visit(expression::operator_::double_::CastInteger* i)
{
    auto result = builder()->addTmp("i", hilti::builder::integer::type(64));
    auto op1 = cg()->hiltiExpression(i->op1());

    if ( ast::checkedCast<type::Integer>(i->type())->signed_() )
        cg()->builder()->addInstruction(result, hilti::instruction::double_::AsSInt, op1);
    else
        cg()->builder()->addInstruction(result, hilti::instruction::double_::AsUInt, op1);

    setResult(result);
}


void CodeBuilder::visit(expression::operator_::double_::CoerceBool* i)
{
    auto result = builder()->addTmp("notnull", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto null = hilti::builder::double_::create(0);
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Unequal, op1, null);
    setResult(result);
}

#if 0
void CodeBuilder::visit(expression::operator_::double_::CoerceInterval* i)
{
    auto result = builder()->addTmp("i", hilti::builder::interval::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::double_::AsInterval, op1);
    setResult(result);
}
#endif

void CodeBuilder::visit(expression::operator_::double_::Equal* i)
{
    auto result = builder()->addTmp("equal", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Equal, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::double_::Greater* i)
{
    auto result = builder()->addTmp("gt", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::double_::Gt, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::double_::Lower* i)
{
    auto result = builder()->addTmp("lt", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::double_::Lt, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::double_::Div* i)
{
    auto result = builder()->addTmp("div", hilti::builder::double_::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::double_::Div, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::double_::Minus* i)
{
    auto result = builder()->addTmp("diff", hilti::builder::double_::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::double_::Sub, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::double_::Mod* i)
{
    auto result = builder()->addTmp("rem", hilti::builder::double_::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::double_::Mod, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::double_::Mult* i)
{
    auto result = builder()->addTmp("prod", hilti::builder::double_::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::double_::Mul, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::double_::Plus* i)
{
    auto result = builder()->addTmp("sum", hilti::builder::double_::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::double_::Add, op1, op2);
    setResult(result);
}


void CodeBuilder::visit(expression::operator_::double_::Power* i)
{
    auto result = builder()->addTmp("pwr", hilti::builder::double_::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::double_::PowDouble, op1, op2);
    setResult(result);
}

