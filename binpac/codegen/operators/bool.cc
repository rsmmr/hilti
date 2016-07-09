
#include "cg-operator-common.h"
#include <binpac/autogen/operators/bool.h>

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
    // Make sure to short-circuit evaluation.

    auto result = builder()->addTmp("and", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1(), std::make_shared<type::Bool>());

    auto branches = cg()->builder()->addIfElse(op1);
    auto op1_true = std::get<0>(branches);
    auto op1_false = std::get<1>(branches);
    auto done = std::get<2>(branches);

    cg()->moduleBuilder()->pushBuilder(op1_true);
    auto op2 = cg()->hiltiExpression(i->op2(), std::make_shared<type::Bool>());
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Assign, op2);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(op1_true);

    cg()->moduleBuilder()->pushBuilder(op1_false);
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Assign,
                                    hilti::builder::boolean::create(false));
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(op1_false);

    cg()->moduleBuilder()->pushBuilder(done);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bool_::LogicalOr* i)
{
    // Make sure to short-circuit evaluation.

    auto result = builder()->addTmp("or", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1(), std::make_shared<type::Bool>());

    auto branches = cg()->builder()->addIfElse(op1);
    auto op1_true = std::get<0>(branches);
    auto op1_false = std::get<1>(branches);
    auto done = std::get<2>(branches);

    cg()->moduleBuilder()->pushBuilder(op1_true);
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Assign,
                                    hilti::builder::boolean::create(true));
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(op1_true);

    cg()->moduleBuilder()->pushBuilder(op1_false);
    auto op2 = cg()->hiltiExpression(i->op2(), std::make_shared<type::Bool>());
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Assign, op2);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(op1_false);

    cg()->moduleBuilder()->pushBuilder(done);
    setResult(result);
}
