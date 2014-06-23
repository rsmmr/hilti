
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

static shared_ptr<Expression> _optionalMgr(CodeGen* cg, shared_ptr<Expression> op)
{
    if ( op )
        return op;

    auto tmgr = builder::timer_mgr::type();
    auto rtmgr = builder::reference::type(tmgr);
    auto n = cg->llvmConstNull(cg->llvmType(rtmgr));
    return builder::codegen::create(rtmgr, n);
}

void StatementBuilder::visit(statement::instruction::timer_mgr::New* i)
{
    CodeGen::expr_list args;
    auto result = cg()->llvmCall("hlt::timer_mgr_new", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::timer_mgr::Advance* i)
{
    CodeGen::expr_list args;
    args.push_back(_optionalMgr(cg(), i->op2()));
    args.push_back(i->op1());
    cg()->llvmCall("hlt::timer_mgr_advance", args);
}

void StatementBuilder::visit(statement::instruction::timer_mgr::AdvanceGlobal* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    cg()->llvmCall("hlt::timer_mgr_advance_global", args);
}

void StatementBuilder::visit(statement::instruction::timer_mgr::Current* i)
{
    CodeGen::expr_list args;
    args.push_back(_optionalMgr(cg(), i->op1()));

    auto result = cg()->llvmCall("hlt::timer_mgr_current", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::timer_mgr::Expire* i)
{
    CodeGen::expr_list args;
    args.push_back(_optionalMgr(cg(), i->op2()));
    args.push_back(i->op1());
    cg()->llvmCall("hlt::timer_mgr_expire", args);
}

void StatementBuilder::visit(statement::instruction::timer_mgr::Schedule* i)
{
    CodeGen::expr_list args;
    args.push_back(_optionalMgr(cg(), i->op3()));
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::timer_mgr_schedule", args);
}

