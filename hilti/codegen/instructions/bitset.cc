
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::bitset::Equal* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());
    auto result = cg()->builder()->CreateICmpEQ(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bitset::Clear* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateXor(op2, cg()->llvmConstInt(-1, 64));
    result = builder()->CreateAnd(op1, result);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bitset::Has* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto and_ = builder()->CreateAnd(op1, op2);
    auto result = builder()->CreateICmpEQ(and_, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bitset::Set* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateOr(op1, op2);

    cg()->llvmStore(i, result);
}

