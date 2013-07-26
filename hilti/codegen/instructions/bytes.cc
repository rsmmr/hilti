
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::bytes::New* i)
{
    CodeGen::expr_list args;
    auto result = cg()->llvmCall("hlt::bytes_new", args);
    cg()->llvmStore(i, result);
}

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

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Equal* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto cmp = cg()->llvmCall("hlt::bytes_cmp", args);
    auto result = cg()->builder()->CreateICmpEQ(cmp, cg()->llvmConstInt(0, 8));

    cg()->llvmStore(i, result);
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

    auto result = cg()->llvmCall("hlt::iterator_bytes_diff", args);

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

    auto result = cg()->llvmCall("hlt::iterator_bytes_is_frozen", args);

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

void StatementBuilder::visit(statement::instruction::iterBytes::Begin* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::bytes_begin", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterBytes::End* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::bytes_end", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterBytes::Incr* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::iterator_bytes_incr", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterBytes::IncrBy* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::iterator_bytes_incr_by", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterBytes::Equal* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::iterator_bytes_eq", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterBytes::Deref* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::iterator_bytes_deref", args);

    cg()->llvmStore(i, result);
}

