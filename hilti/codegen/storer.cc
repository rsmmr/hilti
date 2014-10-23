
#include "../hilti.h"

#include "storer.h"
#include "codegen.h"

using namespace hilti;
using namespace codegen;

Storer::Storer(CodeGen* cg) : CGVisitor<int, llvm::Value*, std::pair<bool, bool>>(cg, "codegen::Storer")
{
}

Storer::~Storer()
{
}

void Storer::llvmStore(shared_ptr<Expression> target, llvm::Value* value, bool plusone, bool dtor_first)
{
    setArg1(value);
    setArg2(std::make_pair(plusone, dtor_first));
    call(target);
}

void Storer::visit(expression::Variable* v)
{
    call(v->variable());
}

void Storer::visit(expression::Parameter* p)
{
    auto val = arg1();
    auto plusone = arg2().first;

    auto param = p->parameter();

    assert(! param->constant());

    // Store in shadow local.
    auto name = "__shadow_" + param->id()->name();
    auto addr = cg()->llvmLocal(name);

    cg()->llvmCreateStore(val, addr);

    if ( plusone )
        cg()->llvmDtor(val, p->type(), false, "storer/parameter");
}

void Storer::visit(expression::CodeGen* c)
{
    auto val = arg1();
    llvm::Value* addr = reinterpret_cast<llvm::Value*>(c->cookie());
    cg()->llvmCreateStore(val, addr);
}

void Storer::visit(variable::Global* v)
{
    auto val = arg1();
    auto plusone = arg2().first;
    auto dtor_first = arg2().second;

    auto addr = cg()->llvmGlobal(v);

    cg()->llvmGCAssign(addr, val, v->type(), plusone, dtor_first);
}

void Storer::visit(variable::Local* v)
{
    auto val = arg1();
    auto plusone = arg2().first;

    auto name = v->internalName();
    assert(name.size());

    auto addr = cg()->llvmLocal(name);
    cg()->llvmCreateStore(val, addr);

    if ( plusone )
        cg()->llvmDtor(val, v->type(), false, "storer/local");
}
