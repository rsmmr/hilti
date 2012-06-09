
#include <hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::string::Equal* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto cmp = cg()->llvmCall("hlt::string_cmp", args);
    auto result = builder()->CreateICmpEQ(cmp, cg()->llvmConstInt(0, 8));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::string::Cmp* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto cmp = cg()->llvmCall("hlt::string_cmp", args);
    auto result = builder()->CreateICmpEQ(cmp, cg()->llvmConstInt(0, 8));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::string::Concat* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::string_concat", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::string::Decode* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::string_decode", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::string::Encode* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::string_encode", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::string::Find* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::string_find", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::string::Length* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::string_len", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::string::Lt* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto cmp = cg()->llvmCall("hlt::string_cmp", args);
    auto result = builder()->CreateICmpSLT(cmp, cg()->llvmConstInt(0, 8));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::string::Substr* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());

    auto result = cg()->llvmCall("hlt::string_substr", args);

    cg()->llvmStore(i, result);
}

