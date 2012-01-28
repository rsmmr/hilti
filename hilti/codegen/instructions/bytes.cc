
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::bytes::Append* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::bytes_append", args);
}

void StatementBuilder::visit(statement::instruction::bytes::Cmp* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::bytes_cmp", args);
}

void StatementBuilder::visit(statement::instruction::bytes::Copy* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::bytes_copy", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Diff* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::bytes_pos_diff", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Empty* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::bytes_empty", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Freeze* i)
{
    auto cone = shared_ptr<constant::Integer>(new constant::Integer(1, 64, i->location()));
    auto one  = shared_ptr<expression::Constant>(new expression::Constant(cone, i->location()));

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(one);
    cg()->llvmCall("hlt::bytes_freeze", args);
}

void StatementBuilder::visit(statement::instruction::bytes::IsFrozenBytes* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::bytes_is_frozen", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::IsFrozenIterBytes* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::bytes_pos_is_frozen", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Length* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::bytes_len", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Offset* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::bytes_offset", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Sub* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::bytes_sub", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Trim* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::bytes_trim", args);
}

void StatementBuilder::visit(statement::instruction::bytes::Unfreeze* i)
{
    auto czero = shared_ptr<constant::Integer>(new constant::Integer(0, 64, i->location()));
    auto zero  = shared_ptr<expression::Constant>(new expression::Constant(czero, i->location()));

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(zero);
    cg()->llvmCall("hlt::bytes_freeze", args);
}

