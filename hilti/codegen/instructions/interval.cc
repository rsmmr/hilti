
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::interval::Equal* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateICmpEQ(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::interval::Add* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateAdd(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::interval::Mul* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateMul(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::interval::AsDouble* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto val = cg()->builder()->CreateUIToFP(op1, cg()->llvmTypeDouble());
    auto result = cg()->builder()->CreateFDiv(val, cg()->llvmConstDouble(1e9));

    cg()->llvmStore(i, result);
}


void StatementBuilder::visit(statement::instruction::interval::FromDouble* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto val = cg()->builder()->CreateFMul(op1, cg()->llvmConstDouble(1e9));
    auto result = cg()->builder()->CreateFPToUI(val, cg()->llvmTypeInt(64));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::interval::AsInt* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto result = cg()->builder()->CreateUDiv(op1, cg()->llvmConstInt(1000000000, 64));
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::interval::Eq* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateICmpEQ(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::interval::Gt* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateICmpSGT(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::interval::Geq* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateICmpSGE(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::interval::Lt* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateICmpSLT(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::interval::Leq* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateICmpSLE(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::interval::Nsecs* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::interval_nsecs", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::interval::Sub* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateSub(op1, op2);

    cg()->llvmStore(i, result);
}
