
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::time::Equal* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateICmpEQ(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::time::Add* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateAdd(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::time::AsDouble* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto val = cg()->builder()->CreateUIToFP(op1, cg()->llvmTypeDouble());
    auto result = cg()->builder()->CreateFDiv(val, cg()->llvmConstDouble(1e9));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::time::AsInt* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto result = cg()->builder()->CreateUDiv(op1, cg()->llvmConstInt(1000000000, 1e9));
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::time::Eq* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateICmpEQ(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::time::Gt* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateICmpSGT(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::time::Lt* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateICmpSLT(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::time::Nsecs* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::time_nsecs", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::time::Sub* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateSub(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::time::Wall* i)
{
    CodeGen::expr_list no_args;
    auto result = cg()->llvmCall("hlt::time_wall", no_args);
    cg()->llvmStore(i, result);
}
