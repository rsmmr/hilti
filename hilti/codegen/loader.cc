
#include "../hilti.h"

#include "loader.h"
#include "codegen.h"

using namespace hilti;
using namespace codegen;

Loader::Loader(CodeGen* cg) : CGVisitor<llvm::Value*>(cg, "codegen::Loader")
{
}

Loader::~Loader()
{
}

llvm::Value* Loader::llvmValue(shared_ptr<Expression> expr, shared_ptr<hilti::Type> coerce_to)
{
    if ( coerce_to )
        expr = expr->coerceTo(coerce_to);

    llvm::Value* result;
    bool success = processOne(expr, &result);
    assert(success);
    return result;
}

llvm::Value* Loader::llvmValue(shared_ptr<Constant> constant)
{
    llvm::Value* result;
    bool success = processOne(constant, &result);
    assert(success);
    return result;
}
void Loader::visit(expression::Variable* v)
{
    call(v->variable());
}

void Loader::visit(variable::Local* v)
{
    auto name = v->id()->name();
    auto addr = cg()->llvmLocal(name);
    setResult(cg()->builder()->CreateLoad(addr));
}

void Loader::visit(variable::Global* v)
{
    auto addr = cg()->llvmGlobal(v);
    setResult(cg()->builder()->CreateLoad(addr));
}

void Loader::visit(expression::Parameter* p)
{
    auto val = cg()->llvmParameter(p->parameter());
    setResult(val);
}

void Loader::visit(expression::Constant* e)
{
    setResult(llvmValue(e->constant()));
}

void Loader::visit(expression::Coerced* e)
{
    auto val = llvmValue(e->expression());
    auto coerced = cg()->llvmCoerceTo(val, e->expression()->type(), e->type());
    setResult(coerced);
}

void Loader::visit(expression::Function* f)
{
    auto val = cg()->llvmFunction(f->function());
    setResult(val);
}

void Loader::visit(expression::CodeGen* c)
{
    llvm::Value* val = reinterpret_cast<llvm::Value*>(c->cookie());
    setResult(val);
}

void Loader::visit(constant::Integer* c)
{
    auto val = cg()->llvmConstInt(c->value(), as<type::Integer>(c->type())->width());
    setResult(val);
}

void Loader::visit(constant::String* s)
{
    auto val = cg()->llvmString(s->value());
    setResult(val);
}

void Loader::visit(constant::Bool* b)
{
    auto val = cg()->llvmConstInt(b->value() ? 1 : 0, 8);
    setResult(val);
}

void Loader::visit(constant::Tuple* t)
{
    std::vector<llvm::Value *> elems;
    for ( auto e : t->value() )
        elems.push_back(cg()->llvmValue(e));

    auto val = cg()->llvmValueStruct(elems);
    setResult(val);
}

void Loader::visit(constant::Reference* r)
{
    // This can only be the null value.
    auto val = cg()->llvmConstNull();
    setResult(val);
}

