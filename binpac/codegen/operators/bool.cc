
#include "cg-operator-common.h"
#include "autogen/operators/bool.h"

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(constant::Bool* b)
{
    auto result = hilti::builder::boolean::create(b->value());
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bool_::Equal* i)
{
    auto result = builder()->addTmp("equal", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1(), std::make_shared<type::Bool>());
    auto op2 = cg()->hiltiExpression(i->op2(), std::make_shared<type::Bool>());
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Equal, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bool_::Not* i)
{
    auto result = builder()->addTmp("not", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1(), std::make_shared<type::Bool>());
    cg()->builder()->addInstruction(result, hilti::instruction::boolean::Not, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bool_::LogicalAnd* i)
{
    auto result = builder()->addTmp("and", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1(), std::make_shared<type::Bool>());
    auto op2 = cg()->hiltiExpression(i->op2(), std::make_shared<type::Bool>());
    cg()->builder()->addInstruction(result, hilti::instruction::boolean::And, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bool_::LogicalOr* i)
{
    auto result = builder()->addTmp("or", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1(), std::make_shared<type::Bool>());
    auto op2 = cg()->hiltiExpression(i->op2(), std::make_shared<type::Bool>());
    cg()->builder()->addInstruction(result, hilti::instruction::boolean::Or, op1, op2);
    setResult(result);
}
