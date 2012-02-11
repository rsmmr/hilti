
#include "../hilti.h"

#include "storer.h"
#include "codegen.h"

using namespace hilti;
using namespace codegen;

Storer::Storer(CodeGen* cg) : CGVisitor<int, llvm::Value*>(cg, "codegen::Storer")
{
}

Storer::~Storer()
{
}

void Storer::llvmStore(shared_ptr<Expression> target, llvm::Value* value)
{
    setArg1(value);
    call(target);
}

void Storer::visit(expression::Variable* v)
{
    call(v->variable());
}

void Storer::visit(expression::Parameter* p)
{
    auto param = p->parameter();

    assert(! param->constant());

    // Store in shadow local.
    auto name = "__shadow_" + param->id()->name();
    auto addr = cg()->llvmLocal(name);

    auto val = arg1();
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
    auto addr = cg()->llvmGlobal(v);
    cg()->llvmCheckedCreateStore(val, addr, true); // Mark as volatile.
}

void Storer::visit(variable::Local* v)
{
    auto val = arg1();
    auto name = v->id()->name();
    auto addr = cg()->llvmLocal(name);
    cg()->llvmCheckedCreateStore(val, addr);
}
