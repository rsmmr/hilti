
#include "../hilti.h"

#include "storer.h"
#include "codegen.h"

using namespace hilti;
using namespace codegen;

Storer::Storer(CodeGen* cg) : CGVisitor<int, llvm::Value*, bool>(cg, "codegen::Storer")
{
}

Storer::~Storer()
{
}

void Storer::llvmStore(shared_ptr<Expression> target, llvm::Value* value, bool plusone)
{
    setArg1(value);
    setArg2(plusone);
    call(target);
}

void Storer::visit(expression::Variable* v)
{
    call(v->variable());
}

void Storer::visit(expression::Parameter* p)
{
    auto val = arg1();
    auto plusone = arg2();

    auto param = p->parameter();

    assert(! param->constant());

    // Store in shadow local.
    auto name = "__shadow_" + param->id()->name();
    auto addr = cg()->llvmLocal(name);

    cg()->llvmUnref(builder()->CreateLoad(addr), p->type());

    if ( ! plusone )
        cg()->llvmRef(val, p->type());

    cg()->llvmCheckedCreateStore(val, addr);
}

void Storer::visit(expression::CodeGen* c)
{
    auto val = arg1();
    llvm::Value* addr = reinterpret_cast<llvm::Value*>(c->cookie());
    cg()->llvmCheckedCreateStore(val, addr);
}

void Storer::visit(variable::Global* v)
{
    auto val = arg1();
    auto plusone = arg2();

    auto addr = cg()->llvmGlobal(v);

    cg()->llvmUnref(builder()->CreateLoad(addr), v->type());

    if ( ! plusone )
        cg()->llvmRef(val, v->type());

    cg()->llvmCheckedCreateStore(val, addr, true); // Mark as volatile.
}

void Storer::visit(variable::Local* v)
{
    auto val = arg1();
    auto plusone = arg2();

    auto name = v->id()->name();
    auto addr = cg()->llvmLocal(name);

    cg()->llvmUnref(builder()->CreateLoad(addr), v->type());

    if ( ! plusone )
        cg()->llvmRef(val, v->type());

    cg()->llvmCheckedCreateStore(val, addr);
}
