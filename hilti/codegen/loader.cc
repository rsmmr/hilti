
#include <algorithm>

#include "../hilti.h"

#include "loader.h"
#include "codegen.h"
#include "util.h"
#include "type-builder.h"

#include "libhilti/enum.h"
#include "libhilti/port.h"

using namespace hilti;
using namespace codegen;

Loader::Loader(CodeGen* cg) : CGVisitor<_LoadResult, shared_ptr<Type>>(cg, "codegen::Loader")
{
}

Loader::~Loader()
{
}

llvm::Value* Loader::normResult(const _LoadResult& result, shared_ptr<Type> type, bool cctor)
{
    if ( cctor ) {
        if ( ! result.cctor )
            cg()->llvmCctor(result.value, type, result.is_ptr, "loader.normResult");
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
    setArg1(coerce_to);
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
    auto name = v->internalName();
    assert(name.size());

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
    CGVisitor<_LoadResult, shared_ptr<Type>>::setResult(result);
}

void Loader::visit(expression::Ctor* e)
{
    _LoadResult result;
    bool success = processOne(e->ctor(), &result);
    CGVisitor<_LoadResult, shared_ptr<Type>>::setResult(result);
}

void Loader::visit(expression::Coerced* e)
{
    auto val = llvmValue(e->expression(), true);
    auto coerced = cg()->llvmCoerceTo(val, e->expression()->type(), e->type());
    setResult(coerced, true, false);
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
    if ( s->value().size() != 0 ) {
        auto addr = cg()->llvmStringPtr(s->value());
        setResult(addr, false, true);
    }

    else {
        // The empty string can be represented by a null pointer but
        // llvmStringPtr() always creates a new object as it needs to return
        // a pointer.
        auto addr = cg()->llvmConstNull(cg()->llvmTypeString());
        setResult(addr, true, false);
    }
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
    auto result = cg()->llvmTuple(t->type(), t->value(), false);
    setResult(result, false, false);
}

void Loader::visit(constant::Unset* t)
{
    // Just a dummy value.
    setResult(cg()->llvmConstInt(0, 1), false, false);
}

void Loader::visit(constant::Reference* r)
{
    // This can only be the null value.
    auto dtype = arg1() ? arg1() : r->type();
    auto val = cg()->llvmConstNull(cg()->llvmType(dtype));
    setResult(val, false, false);
}

static CodeGen::constant_list _loadAddressVal(CodeGen* cg, const constant::AddressVal& val, int width)
{
    uint64_t a, b;

    switch ( val.family ) {
     case constant::AddressVal::IPv4: {
        uint32_t* p = (uint32_t*) &val.in.in4.s_addr;
        a = 0;
        b = codegen::util::ntoh32(p[0]);
        break;
     }

     case constant::AddressVal::IPv6: {
        uint64_t* p = (uint64_t*) &val.in.in6.s6_addr;
        a = codegen::util::ntoh64(p[0]);
        b = codegen::util::ntoh64(p[1]);
        break;
     }

     default:
        assert(false);
    }

    if ( width > 0 )
        a &= (0xFFFFFFFFFFFFFFFFu << (64 - (std::min(64, width))));
    else
        a = 0;

    if ( width <= 64 )
        b = 0;
    else
        b &= (0xFFFFFFFFFFFFFFFFu << (64 - (width - 64)));

    CodeGen::constant_list elems;
    elems.push_back(cg->llvmConstInt(a, 64));
    elems.push_back(cg->llvmConstInt(b, 64));

    return elems;
}

void Loader::visit(constant::Address* c)
{
    auto elems = _loadAddressVal(cg(), c->value(), 128);
    auto val = cg()->llvmConstStruct(elems);
    setResult(val, false, false);
}

void Loader::visit(constant::Network* c)
{
    auto width = c->width();

    if ( c->prefix().family == constant::AddressVal::IPv4 )
        width += 96;

    auto elems = _loadAddressVal(cg(), c->prefix(), width);

    elems.push_back(cg()->llvmConstInt(width, 8));

    auto val = cg()->llvmConstStruct(elems);
    setResult(val, false, false);
}

void Loader::visit(constant::Double* c)
{
    auto val = cg()->llvmConstDouble(c->value());
    setResult(val, false, false);
}

void Loader::visit(constant::Bitset* c)
{
    auto bstype = ast::as<type::Bitset>(c->type());
    assert(bstype);

    auto bits = c->value();

    uint64_t i = 0;

    for ( auto b : bits )
        i |= (1 << bstype->labelBit(b));

    auto val = cg()->llvmConstInt(i, 64);

    setResult(val, false, false);
}

void Loader::visit(constant::Enum* c)
{
    auto etype = ast::as<type::Enum>(c->type());
    assert(etype);

    auto bits = c->value();

    int i = etype->labelValue(c->value());
    uint8_t flags = HLT_ENUM_HAS_VAL;

    if ( i < 0 ) {
        // Undef.
        flags = HLT_ENUM_UNDEF;
        i = 0;
    }

    CodeGen::constant_list elems;
    elems.push_back(cg()->llvmConstInt(flags, 8));
    elems.push_back(cg()->llvmConstInt(i, 64));

    auto val = cg()->llvmConstStruct(elems);

    setResult(val, false, false);
}

void Loader::visit(constant::Interval* c)
{
    auto val = cg()->llvmConstInt(c->value(), 64);
    setResult(val, false, false);
}

void Loader::visit(constant::Time* c)
{
    auto val = cg()->llvmConstInt(c->value(), 64);
    setResult(val, false, false);
}

void Loader::visit(constant::Port* c)
{
    auto p = c->value();

    int proto = 0;

    switch ( p.proto ) {
     case constant::PortVal::TCP:
        proto = HLT_PORT_TCP;
        break;

     case constant::PortVal::UDP:
        proto = HLT_PORT_UDP;
        break;

     default:
        internalError(c, "unknown port protocol");
    }

    CodeGen::constant_list elems;
    elems.push_back(cg()->llvmConstInt(p.port, 16));
    elems.push_back(cg()->llvmConstInt(proto, 8));
    auto val = cg()->llvmConstStruct(elems, true);

    setResult(val, false, false);
}

void Loader::visit(ctor::Bytes* c)
{
    std::vector<llvm::Constant*> vec_data;

    for ( auto c : c->value() )
        vec_data.push_back(cg()->llvmConstInt(c, 8));

    auto array = cg()->llvmConstArray(cg()->llvmTypeInt(8), vec_data);
    llvm::Constant* data = cg()->llvmAddConst("bytes", array);
    data = llvm::ConstantExpr::getBitCast(data, cg()->llvmTypePtr());

    CodeGen::value_list args;
    args.push_back(data);
    args.push_back(cg()->llvmConstInt(c->value().size(), 64));

    auto val = cg()->llvmCallC("hlt_bytes_new_from_data_copy", args, true);

    setResult(val, true, false);
}

static shared_ptr<Expression> _tmgrNull(CodeGen* cg)
{
    auto refTimerMgr = builder::reference::type(builder::timer_mgr::type());
    auto null = cg->llvmConstNull(cg->llvmTypePtr(cg->llvmLibType("hlt.timer_mgr")));
    return builder::codegen::create(refTimerMgr, null);
}

void Loader::visit(ctor::List* c)
{
    auto rtype = ast::as<type::Reference>(c->type())->argType();
    auto etype = ast::as<type::List>(rtype)->argType();
    assert(etype);

    auto op1 = builder::type::create(etype);
    auto op2 = _tmgrNull(cg());

    CodeGen::expr_list args = {op1, op2};
    auto list = cg()->llvmCall("hlt::list_new", args);

    auto listop = builder::codegen::create(c->type(), list);

    for ( auto e : c->elements() ) {
        e = e->coerceTo(etype);
        CodeGen::expr_list args = { listop, e };
        cg()->llvmCall("hlt::list_push_back", args);
    }

    setResult(list, true, false);
}

void Loader::visit(ctor::Set* c)
{
    auto rtype = ast::as<type::Reference>(c->type())->argType();
    auto etype = ast::as<type::Set>(rtype)->argType();
    assert(etype);

    auto op1 = builder::type::create(etype);
    auto op2 = _tmgrNull(cg());

    CodeGen::expr_list args = {op1, op2};
    auto set = cg()->llvmCall("hlt::set_new", args);

    auto setop = builder::codegen::create(c->type(), set);

    for ( auto e : c->elements() ) {
        e = e->coerceTo(etype);
        CodeGen::expr_list args = { setop, e };
        cg()->llvmCall("hlt::set_insert", args);
    }

    setResult(set, true, false);
}

void Loader::visit(ctor::Vector* c)
{
    auto rtype = ast::as<type::Reference>(c->type())->argType();
    auto etype = ast::as<type::Vector>(rtype)->argType();
    assert(etype);

    auto def = builder::codegen::create(etype, cg()->typeInfo(etype)->init_val);
    auto op2 = _tmgrNull(cg());

    CodeGen::expr_list args = {def, op2};
    auto vec = cg()->llvmCall("hlt::vector_new", args);

    auto vecop = builder::codegen::create(c->type(), vec);

    for ( auto e : c->elements() ) {
        e = e->coerceTo(etype);
        CodeGen::expr_list args = { vecop, e };
        cg()->llvmCall("hlt::vector_push_back", args);
    }

    setResult(vec, true, false);
}

void Loader::visit(ctor::Map* c)
{
    assert(false);
    // Not implemented;
    // setResult(val, true, false);
}

void Loader::visit(ctor::RegExp* c)
{
    auto patterns = c->patterns();

    auto top = ast::as<type::Reference>(c->type())->argType();
    CodeGen::expr_list args = { builder::type::create(top)};
    auto regexp = cg()->llvmCall("hlt::regexp_new", args);
    auto op1 = builder::codegen::create(c->type(), regexp);

    if ( patterns.size() == 1 ) {
        // Just one pattern, we use regexp_compile().
        auto pattern = patterns.front();
        CodeGen::expr_list args;
        args.push_back(op1);
        args.push_back(builder::string::create(pattern.first));
        cg()->llvmCall("hlt::regexp_compile", args);
    }

    else {
        // More than one pattern. We built a list of the patterns and then
        // call compile_set.
        auto ttmgr = builder::reference::type(builder::timer_mgr::type());
        auto tmgr = builder::codegen::create(ttmgr, cg()->llvmConstNull(cg()->llvmTypePtr(cg()->llvmLibType("hlt.timer_mgr"))));
        CodeGen::expr_list args = { builder::type::create(builder::string::type()), tmgr };
        auto list = cg()->llvmCall("hlt::list_new", args);
        auto ltype = builder::reference::type(builder::list::type(builder::string::type()));

        for ( auto p : patterns ) {
            args = { builder::codegen::create(ltype, list),
                     builder::string::create(p.first) };
            cg()->llvmCall("hlt::list_push_back", args);
        }

        args = { op1, builder::codegen::create(ltype, list) };
        cg()->llvmCall("hlt::regexp_compile_set", args);
        cg()->llvmDtor(list, ltype, false, "ctor::RegExp");
    }

    setResult(regexp, true, false);
}
