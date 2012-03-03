
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::timer_mgr::New* i)
{
    CodeGen::expr_list args;
    auto result = cg()->llvmCall("hlt::timer_mgr_new", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::timer_mgr::Advance* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::timer_mgr_advance", args);
}

void StatementBuilder::visit(statement::instruction::timer_mgr::Current* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::timer_mgr_current", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::timer_mgr::Expire* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::timer_mgr_expire", args);
}

void StatementBuilder::visit(statement::instruction::timer_mgr::Schedule* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());
    cg()->llvmCall("hlt::timer_mgr_schedule", args);
}

