
#include "../hilti.h"

#include "loader.h"
#include "codegen.h"

using namespace hilti;
using namespace codegen;

Loader::Loader(CodeGen* cg) : CGVisitor<_LoadResult>(cg, "codegen::Loader")
{
}

Loader::~Loader()
{
}

llvm::Value* Loader::normResult(const _LoadResult& result, shared_ptr<Type> type, bool cctor)
{
    if ( cctor ) {
        if ( ! result.cctor )
            cg()->llvmCctor(result.value, type, result.is_ptr);
    }

    else {
        if ( result.cctor )
            cg()->llvmDtorAfterInstruction(result.value, type, result.is_ptr);
    }

    if ( result.is_ptr )
        return cg()->builder()->CreateLoad(result.value);
    else
        return result.value;
}

llvm::Value* Loader::llvmValue(shared_ptr<Expression> expr, bool cctor, shared_ptr<hilti::Type> coerce_to)
{
    if ( coerce_to )
        expr = expr->coerceTo(coerce_to);

    _cctor = cctor;
    _LoadResult result;
    bool success = processOne(expr, &result);
    assert(success);

    return normResult(result, expr->type(), cctor);
}

llvm::Value* Loader::llvmValue(shared_ptr<Constant> constant, bool cctor)
{
    _cctor = cctor;
    _LoadResult result;
    bool success = processOne(constant, &result);
    assert(success);

    return normResult(result, constant->type(), cctor);
}

llvm::Value* Loader::llvmValue(shared_ptr<Ctor> ctor, bool cctor)
{
    _cctor = cctor;
    _LoadResult result;
    bool success = processOne(ctor, &result);
    assert(success);

    return normResult(result, ctor->type(), cctor);
}

void Loader::visit(expression::Variable* v)
{
    call(v->variable());
}

void Loader::visit(variable::Local* v)
{
    auto name = v->id()->name();
    auto addr = cg()->llvmLocal(name);
    setResult(addr, false, true);
}

void Loader::visit(variable::Global* v)
{
    auto addr = cg()->llvmGlobal(v);
    auto result = cg()->builder()->CreateLoad(addr);
    setResult(addr, false, true);
}

void Loader::visit(expression::Parameter* p)
{
    auto param = p->parameter();

    llvm::Value* val = 0;

    if ( param->constant() ) {
        val = cg()->llvmParameter(param);
        setResult(val, false, false);
    }

    else {
        // Load shadow local.
        auto name = "__shadow_" + param->id()->name();
        auto addr = cg()->llvmLocal(name);
        setResult(addr, false, true);
    }
}

void Loader::visit(expression::Constant* e)
{
    _LoadResult result;
    bool success = processOne(e->constant(), &result);
    CGVisitor<_LoadResult>::setResult(result);
}

void Loader::visit(expression::Ctor* e)
{
    _LoadResult result;
    bool success = processOne(e->ctor(), &result);
    CGVisitor<_LoadResult>::setResult(result);
}

void Loader::visit(expression::Coerced* e)
{
    auto val = llvmValue(e->expression(), _cctor);
    auto coerced = cg()->llvmCoerceTo(val, e->expression()->type(), e->type());
    setResult(coerced, _cctor, false);
}

void Loader::visit(expression::Function* f)
{
    auto val = cg()->llvmFunction(f->function());
    setResult(val, false, false);
}

void Loader::visit(expression::CodeGen* c)
{
    llvm::Value* result = reinterpret_cast<llvm::Value*>(c->cookie());
    setResult(result, false, false);
}

void Loader::visit(constant::Integer* c)
{
    auto val = cg()->llvmConstInt(c->value(), as<type::Integer>(c->type())->width());
    setResult(val, false, false);
}

void Loader::visit(constant::String* s)
{
    auto addr = cg()->llvmStringPtr(s->value());
    setResult(addr, false, true);
}

void Loader::visit(constant::Bool* b)
{
    auto val = cg()->llvmConstInt(b->value() ? 1 : 0, 1);
    setResult(val, false, false);
}

void Loader::visit(constant::Label* l)
{
    auto b = cg()->builderForLabel(l->value());
    auto result = b->GetInsertBlock();
    setResult(result, false, false);
}

void Loader::visit(constant::Tuple* t)
{
    std::vector<llvm::Value *> elems;
    for ( auto e : t->value() )
        elems.push_back(cg()->llvmValue(e));

    auto result = cg()->llvmValueStruct(elems);
    setResult(result, false, false);
}

void Loader::visit(constant::Reference* r)
{
    // This can only be the null value.
    auto val = cg()->llvmConstNull();
    setResult(val, false, false);
}

void Loader::visit(ctor::Bytes* b)
{
    std::vector<llvm::Constant*> vec_data;
    for ( auto c : b->value() )
        vec_data.push_back(cg()->llvmConstInt(c, 8));

    auto array = cg()->llvmConstArray(cg()->llvmTypeInt(8), vec_data);
    llvm::Constant* data = cg()->llvmAddConst("bytes", array);
    data = llvm::ConstantExpr::getBitCast(data, cg()->llvmTypePtr());

    CodeGen::value_list args;
    args.push_back(data);
    args.push_back(cg()->llvmConstInt(b->value().size(), 64));

    auto val = cg()->llvmCallC("hlt_bytes_new_from_data_copy", args, true);

    setResult(val, true, false);
}
