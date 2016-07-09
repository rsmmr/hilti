
#include <algorithm>

#include "../hilti.h"

#include "codegen.h"
#include "loader.h"
#include "type-builder.h"
#include "util.h"

#include "libhilti/enum.h"
#include "libhilti/port.h"
#include "libhilti/regexp.h"

using namespace hilti;
using namespace codegen;

Loader::Loader(CodeGen* cg) : CGVisitor<_LoadResult, shared_ptr<Type>>(cg, "codegen::Loader")
{
}

Loader::~Loader()
{
}

llvm::Value* Loader::normResult(const _LoadResult& result, shared_ptr<Type> type, bool cctor,
                                llvm::Value* dst)
{
    if ( ! dst ) {
        if ( cctor ) {
            if ( ! result.cctor )
                cg()->llvmCctor(result.value, type, result.is_ptr, "loader.normResult");
        }

        else {
            if ( result.cctor )
                cg()->llvmDtorAfterInstruction(result.value, type, result.is_ptr, result.is_hoisted,
                                               "Loader.normResult");
        }
    }

    if ( dst ) {
        if ( ! result.stored_in_dst ) {
            if ( result.is_ptr ) {
                auto tmp = cg()->builder()->CreateLoad(result.value);
                cg()->llvmCreateStore(tmp, dst);
            }

            else
                cg()->llvmCreateStore(result.value, dst);
        }

        return nullptr;
    }

    else {
        if ( result.is_ptr )
            return cg()->builder()->CreateLoad(result.value);
        else
            return result.value;
    }
}

llvm::Value* Loader::llvmValue(shared_ptr<Expression> expr, bool cctor,
                               shared_ptr<hilti::Type> coerce_to)
{
    if ( coerce_to )
        expr = expr->coerceTo(coerce_to);

    _cctor = cctor;
    _dst = nullptr;
    _LoadResult result;
    setArg1(coerce_to);
    bool success = processOne(expr, &result);
    assert(success);
    _UNUSED(success);

    return normResult(result, expr->type(), cctor, nullptr);
}

void Loader::llvmValueInto(llvm::Value* dst, shared_ptr<Expression> expr, bool cctor,
                           shared_ptr<hilti::Type> coerce_to)
{
    if ( coerce_to )
        expr = expr->coerceTo(coerce_to);

    _cctor = cctor;
    _dst = dst;
    _LoadResult result;
    setArg1(coerce_to);
    bool success = processOne(expr, &result);
    assert(success);
    _UNUSED(success);

    normResult(result, expr->type(), cctor, dst);
}

llvm::Value* Loader::llvmValueAddress(shared_ptr<Expression> expr)
{
    _cctor = false;
    _dst = nullptr;
    _LoadResult result;
    setArg1(nullptr);
    bool success = processOne(expr, &result);
    assert(success);
    _UNUSED(success);

    return result.is_ptr ? result.value : nullptr;
}

llvm::Value* Loader::llvmValue(shared_ptr<Constant> constant, bool cctor)
{
    _cctor = cctor;
    _dst = nullptr;
    _LoadResult result;
    bool success = processOne(constant, &result);
    assert(success);
    _UNUSED(success);

    return normResult(result, constant->type(), cctor, nullptr);
}

void Loader::llvmValueInto(llvm::Value* dst, shared_ptr<Constant> constant, bool cctor)
{
    _cctor = cctor;
    _dst = dst;
    _LoadResult result;
    bool success = processOne(constant, &result);
    assert(success);
    _UNUSED(success);

    normResult(result, constant->type(), cctor, dst);
}

llvm::Value* Loader::llvmValue(shared_ptr<Ctor> ctor, bool cctor)
{
    _cctor = cctor;
    _dst = 0;
    _LoadResult result;
    bool success = processOne(ctor, &result);
    assert(success);
    _UNUSED(success);

    return normResult(result, ctor->type(), cctor, nullptr);
}

void Loader::llvmValueInto(llvm::Value* dst, shared_ptr<Ctor> ctor, bool cctor)
{
    _cctor = cctor;
    _dst = dst;
    _LoadResult result;
    bool success = processOne(ctor, &result);
    assert(success);
    _UNUSED(success);

    normResult(result, ctor->type(), cctor, dst);
}

void Loader::visit(expression::Variable* v)
{
    call(v->variable());
}

void Loader::visit(expression::Type* v)
{
    auto ti = cg()->llvmRtti(v->type());
    setResult(ti, false, false);
}

void Loader::visit(variable::Local* v)
{
    auto name = v->internalName();
    assert(name.size());

    auto hoisted = v->attributes().has(attribute::HOIST);
    auto addr = cg()->llvmLocal(name);
    setResult(addr, false, ! hoisted, hoisted);
}

void Loader::visit(variable::Global* v)
{
    auto addr = cg()->llvmGlobal(v);
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
    processOne(e->constant(), &result);
    CGVisitor<_LoadResult, shared_ptr<Type>>::setResult(result);
}

void Loader::visit(expression::Ctor* e)
{
    _LoadResult result;
    processOne(e->ctor(), &result);
    CGVisitor<_LoadResult, shared_ptr<Type>>::setResult(result);
}

void Loader::visit(expression::Default* e)
{
    auto val = cg()->typeInfo(e->type())->init_val;
    setResult(val, true, false);
}

void Loader::visit(expression::Coerced* e)
{
    auto val = llvmValue(e->expression(), false);
    auto coerced = cg()->llvmCoerceTo(val, e->expression()->type(), e->type(), false);
    setResult(coerced, false, false);
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
    auto val =
        cg()->llvmConstInt(c->value(), ast::rtti::checkedCast<type::Integer>(c->type())->width());
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

void Loader::visit(constant::CAddr* b)
{
    auto val = cg()->llvmConstNull(cg()->llvmTypePtr());
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
    if ( ast::rtti::isA<type::Unset>(t->type()) )
        // Just a dummy value.
        setResult(cg()->llvmConstInt(0, 1), false, false);
    else
        setResult(cg()->llvmInitVal(t->type()), false, false);
}

void Loader::visit(constant::Union* c)
{
    auto dtype = arg1();

    // TODO: Should factor this out into codegen, and then use from
    // instructions/unit.cc as well.
    auto utype = ast::rtti::tryCast<type::Union>(c->type());
    auto data_type = llvm::cast<llvm::StructType>(cg()->llvmType(utype))->getElementType(1);

    shared_ptr<type::union_::Field> field;

    if ( c->id() )
        field = utype->lookup(c->id());

    else if ( c->expression() ) {
        auto fields = utype->fields(c->expression()->type());
        assert(fields.size() == 1);
        field = fields.front();
    }

    if ( ! field ) {
        auto t = ast::rtti::isA<type::Union>(dtype) ?
                     dtype :
                     std::make_shared<type::Union>(type::Union::type_list());
        auto ht = cg()->llvmType(t);
        llvm::Value* none = cg()->llvmConstNull(ht);
        none = cg()->llvmInsertValue(none, cg()->llvmConstInt(-1, 32), 0);
        setResult(none, false, false);
        return;
    }

    int fidx = -1;

    if ( field ) {
        for ( auto f : utype->fields() ) {
            fidx++;

            if ( f == field )
                break;
        }

        assert(fidx < utype->fields().size());
    }

    auto op = cg()->llvmValue(c->expression());
    auto val = cg()->llvmReinterpret(op, data_type);
    auto idx = cg()->llvmConstInt(fidx, 32);

    CodeGen::value_list elems = {idx, val};
    auto result = cg()->llvmValueStruct(elems);
    setResult(result, false, false);
}

void Loader::visit(constant::Reference* r)
{
    // This can only be the null value.
    auto dtype = arg1() ? arg1() : r->type();
    auto val = cg()->llvmConstNull(cg()->llvmType(dtype));
    setResult(val, false, false);
}

static CodeGen::constant_list _loadAddressVal(CodeGen* cg, const constant::AddressVal& val,
                                              int width)
{
    uint64_t a, b;

    switch ( val.family ) {
    case constant::AddressVal::IPv4: {
        uint32_t* p = (uint32_t*)&val.in.in4.s_addr;
        a = 0;
        b = codegen::util::ntoh32(p[0]);
        break;
    }

    case constant::AddressVal::IPv6: {
        uint64_t* p = (uint64_t*)&val.in.in6.s6_addr;
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
    auto bstype = ast::rtti::tryCast<type::Bitset>(c->type());
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
    auto etype = ast::rtti::checkedCast<type::Enum>(c->type());
    ;

    auto bits = c->value();

    int i = etype->labelValue(c->value());
    uint8_t flags = HLT_ENUM_HAS_VAL;

    if ( i < 0 ) {
        // Undef.
        flags = HLT_ENUM_UNDEF;
        i = 0;
    }

    CodeGen::constant_list elems;
    elems.push_back(cg()->llvmConstInt(flags, 64));
    elems.push_back(cg()->llvmConstInt(i, 64));

    auto val = cg()->llvmConstStruct(elems, true);

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

    for ( auto i : c->value() )
        vec_data.push_back(cg()->llvmConstInt(i, 8));

    auto array = cg()->llvmConstArray(cg()->llvmTypeInt(8), vec_data);
    llvm::Constant* data = cg()->llvmAddConst("bytes", array);
    data = llvm::ConstantExpr::getBitCast(data, cg()->llvmTypePtr());

    if ( ! _dst ) {
        CodeGen::value_list args = {data, cg()->llvmConstInt(c->value().size(), 64)};
        auto val = cg()->llvmCallC("hlt_bytes_new_from_data_copy", args, true, false);
        setResult(val, false, false);
    }

    else {
        CodeGen::value_list args = {_dst, data, cg()->llvmConstInt(c->value().size(), 64)};
        cg()->llvmCallC("hlt_bytes_new_from_data_copy_hoisted", args, true, false);
        setResult(nullptr, false, false, true);
    }
}

static shared_ptr<Expression> _tmgrNull(CodeGen* cg)
{
    auto refTimerMgr = builder::reference::type(builder::timer_mgr::type());
    auto null = cg->llvmConstNull(cg->llvmTypePtr(cg->llvmLibType("hlt.timer_mgr")));
    return builder::codegen::create(refTimerMgr, null);
}

void Loader::visit(ctor::List* c)
{
    auto rtype = ast::rtti::tryCast<type::Reference>(c->type())->argType();
    auto etype = ast::rtti::checkedCast<type::List>(rtype)->argType();
    ;

    auto op1 = builder::type::create(etype);
    auto op2 = _tmgrNull(cg());

    CodeGen::expr_list args = {op1, op2};
    auto list = cg()->llvmCall("hlt::list_new", args, false);

    auto listop = builder::codegen::create(c->type(), list);

    for ( auto e : c->elements() ) {
        e = e->coerceTo(etype);
        CodeGen::expr_list args = {listop, e};
        cg()->llvmCall("hlt::list_push_back", args, false);
    }

    setResult(list, false, false);
}

void Loader::visit(ctor::Set* c)
{
    auto rtype = ast::rtti::tryCast<type::Reference>(c->type())->argType();
    auto etype = ast::rtti::tryCast<type::Set>(rtype)->argType();
    assert(etype);

    auto op1 = builder::type::create(etype);
    auto op2 = _tmgrNull(cg());

    CodeGen::expr_list args = {op1, op2};
    auto set = cg()->llvmCall("hlt::set_new", args);

    auto setop = builder::codegen::create(c->type(), set);

    for ( auto e : c->elements() ) {
        e = e->coerceTo(etype);
        CodeGen::expr_list args = {setop, e};
        cg()->llvmCall("hlt::set_insert", args);
    }

    setResult(set, false, false);
}

void Loader::visit(ctor::Vector* c)
{
    auto rtype = ast::rtti::tryCast<type::Reference>(c->type())->argType();
    auto etype = ast::rtti::checkedCast<type::Vector>(rtype)->argType();
    ;

    auto def = builder::codegen::create(etype, cg()->typeInfo(etype)->init_val);
    auto op2 = _tmgrNull(cg());

    CodeGen::expr_list args = {def, op2};
    auto vec = cg()->llvmCall("hlt::vector_new", args);

    auto vecop = builder::codegen::create(c->type(), vec);

    for ( auto e : c->elements() ) {
        e = e->coerceTo(etype);
        CodeGen::expr_list args = {vecop, e};
        cg()->llvmCall("hlt::vector_push_back", args);
    }

    setResult(vec, false, false);
}

void Loader::visit(ctor::Map* c)
{
    auto rtype = ast::rtti::tryCast<type::Reference>(c->type())->argType();
    auto ktype = ast::rtti::tryCast<type::Map>(rtype)->keyType();
    auto vtype = ast::rtti::tryCast<type::Map>(rtype)->valueType();
    assert(ktype && vtype);

    auto op1 = builder::type::create(ktype);
    auto op2 = builder::type::create(vtype);
    auto op3 = _tmgrNull(cg());

    CodeGen::expr_list args = {op1, op2, op3};
    auto map = cg()->llvmCall("hlt::map_new", args);

    auto mapop = builder::codegen::create(c->type(), map);

    for ( auto e : c->elements() ) {
        auto k = e.first->coerceTo(ktype);
        auto v = e.second->coerceTo(vtype);
        CodeGen::expr_list args = {mapop, k, v};
        cg()->llvmCall("hlt::map_insert", args);
    }

    if ( auto def = c->default_() ) {
        auto rtype = ast::rtti::tryCast<type::Reference>(def->type());

        if ( ! (rtype && ast::rtti::isA<type::Callable>(rtype->argType())) )
            def = c->default_()->coerceTo(vtype);

        args = {mapop, def};
        cg()->llvmCall("hlt::map_default", args);
    }

    setResult(map, false, false);
}

void Loader::visit(ctor::RegExp* c)
{
    int flags = 0;

    if ( c->attributes().has(attribute::NOSUB) )
        flags |= HLT_REGEXP_NOSUB;

    if ( c->attributes().has(attribute::FIRSTMATCH) )
        flags |= HLT_REGEXP_FIRST_MATCH;

    CodeGen::expr_list args = {builder::integer::create(flags)};
    auto regexp = cg()->llvmCall("hlt::regexp_new", args);

    auto op1 = builder::codegen::create(c->type(), regexp);

    auto patterns = c->patterns();

    if ( patterns.size() == 1 ) {
        // Just one pattern, we use regexp_compile().
        auto pattern = patterns.front();
        CodeGen::expr_list args;
        args.push_back(op1);
        args.push_back(builder::string::create(pattern));
        cg()->llvmCall("hlt::regexp_compile", args);
    }

    else {
        // More than one pattern. We built a list of the patterns and then
        // call compile_set.
        auto ttmgr = builder::reference::type(builder::timer_mgr::type());
        auto tmgr = builder::codegen::create(ttmgr, cg()->llvmConstNull(cg()->llvmTypePtr(
                                                        cg()->llvmLibType("hlt.timer_mgr"))));
        CodeGen::expr_list args = {builder::type::create(builder::string::type()), tmgr};
        auto list = cg()->llvmCall("hlt::list_new", args);
        auto ltype = builder::reference::type(builder::list::type(builder::string::type()));

        for ( auto p : patterns ) {
            args = {builder::codegen::create(ltype, list), builder::string::create(p)};
            cg()->llvmCall("hlt::list_push_back", args);
        }

        args = {op1, builder::codegen::create(ltype, list)};
        cg()->llvmCall("hlt::regexp_compile_set", args);
    }

    setResult(regexp, false, false);
}

void Loader::visit(ctor::Callable* c)
{
    auto func = ast::rtti::checkedCast<expression::Function>(c->function())->function();
    auto hook = ast::rtti::tryCast<Hook>(func);
    auto ftype = func->type();
    auto tuple = ::builder::tuple::create(c->arguments());

    CodeGen::expr_list params;
    cg()->prepareCall(c->function(), tuple, &params, false);

    if ( hook ) {
        auto htype = ast::rtti::checkedCast<type::Hook>(ftype);
        auto callable = cg()->llvmCallableBind(hook, params, false, false);
        setResult(callable, false, false);
    }

    else {
        auto callable =
            cg()->llvmCallableBind(cg()->llvmValue(c->function()), ftype, params, false, false);
        setResult(callable, false, false);
    }
}
