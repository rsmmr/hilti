
#include "../hilti.h"

#include "type-builder.h"
#include "codegen.h"

#include "../../libhilti/rtti.h"

using namespace hilti;
using namespace codegen;

PointerMap::PointerMap(CodeGen* cg, hilti::Type* type)
{
    _cg = cg;

    auto tl = type::asTrait<type::trait::TypeList>(type);

    if ( ! tl )
        cg->internalError("Type passed to PointerMap TypeList ctor is not a TypeList");

    std::vector<llvm::Type*> fields;

    if ( type::hasTrait<type::trait::GarbageCollected>(type) )
        fields.push_back(cg->llvmLibType("hlt.ref_cnt"));

    std::list<int> ptr_indices;

    for ( auto t : tl->typeList() ) {
        if ( type::hasTrait<type::trait::GarbageCollected>(t) )
            ptr_indices.push_back(fields.size());
        fields.push_back(cg->llvmType(t));
    }

    auto llvm_fields = cg->llvmTypeStruct("ptrmap", fields);

    auto null = cg->llvmConstNull(cg->llvmTypePtr(llvm_fields));
    auto zero = cg->llvmGEPIdx(0);

    for ( auto i : ptr_indices ) {
        auto offset = cg->llvmGEP(null, zero, cg->llvmGEPIdx(i));
        _offsets.push_back(llvm::ConstantExpr::getPtrToInt(offset, cg->llvmTypeInt(16)));
    }
}

llvm::Constant* PointerMap::llvmMap()
{
    _offsets.push_back(_cg->llvmConstInt(HLT_PTR_MAP_END, 16));
    auto cval = _cg->llvmConstArray(_offsets);
    return _cg->llvmAddConst("ptrmap", cval);
}

TypeBuilder::TypeBuilder(CodeGen* cg) : CGVisitor<TypeInfo *>(cg, "codegen::TypeBuilder")
{
}

TypeBuilder::~TypeBuilder()
{
}

shared_ptr<TypeInfo> TypeBuilder::typeInfo(shared_ptr<hilti::Type> type)
{
    auto i = _ti_cache.find(type);

    if ( i != _ti_cache.end() )
        // Already computed.
        return i->second;

    setDefaultResult(nullptr);
    TypeInfo* result;
    bool success = processOne(type, &result);
    assert(success);

    shared_ptr<TypeInfo> ti(result);

    if ( ! ti )
        internalError(::util::fmt("typeInfo() not available for type '%s'", type->repr().c_str()));

    if ( ! ti->llvm_type && ti->init_val )
        ti->llvm_type = ti->init_val->getType();

    if ( ti->id == HLT_TYPE_ERROR )
        internalError(::util::fmt("type info for type %s does not set an RTTI id", type->repr().c_str()));

    if ( ast::isA<type::HiltiType>(type) && ! ti->llvm_type )
        internalError(::util::fmt("type info for %s does not define an llvm_type", ti->name.c_str()));

    if ( ast::isA<type::ValueType>(type) && ! ti->init_val )
        internalError(::util::fmt("type info for %s does not define an init value", ti->name.c_str()));

#if 0
    if ( type::hasTrait<type::trait::GarbageCollected>(type) && ! ti->ptr_map )
        internalError(::util::fmt("type info for %s does not define a ptr map", ti->name.c_str()));

    if ( type::hasTrait<type::trait::GarbageCollected>(type) && ! ti->dtor )
        internalError(::util::fmt("type info for %s does not define a dtor function", ti->name.c_str()));
#endif

    if ( ! ti->name.size() )
        ti->name = type->repr();

    _ti_cache.insert(make_tuple(type, ti));
    return ti;
}

llvm::Type* TypeBuilder::llvmType(shared_ptr<Type> type)
{
    auto ti = typeInfo(type);
    assert(ti && ti->llvm_type);
    return ti->llvm_type;
}

llvm::Constant* TypeBuilder::_lookupFunction(const string& name)
{
    if ( name.size() == 0 )
        return cg()->llvmConstNull(cg()->llvmTypePtr());

    auto func = cg()->llvmFunction(name);
    return llvm::ConstantExpr::getBitCast(func, cg()->llvmTypePtr());
}

llvm::Constant* TypeBuilder::llvmRtti(shared_ptr<hilti::Type> type)
{
    std::vector<llvm::Constant*> vals;

    auto ti = typeInfo(type);

    // Ok if not the right type here.
    shared_ptr<type::trait::Parameterized> ptype = std::dynamic_pointer_cast<type::trait::Parameterized>(ti->type);

    vals.push_back(cg()->llvmConstInt(ti->id, 16));
    vals.push_back(llvm::ConstantExpr::getTrunc(cg()->llvmSizeOf(ti->init_val), cg()->llvmTypeInt(16)));
    vals.push_back(cg()->llvmConstAsciizPtr(ti->name));
    vals.push_back(cg()->llvmConstInt((ptype ? ptype->parameters().size() : 0), 16));
    vals.push_back(ti->aux ? ti->aux : cg()->llvmConstNull(cg()->llvmTypePtr()));
    vals.push_back(ti->ptr_map ? ti->ptr_map : cg()->llvmConstNull(cg()->llvmTypePtr()));
    vals.push_back(_lookupFunction(ti->to_string));
    vals.push_back(_lookupFunction(ti->to_int64));
    vals.push_back(_lookupFunction(ti->to_double));
    vals.push_back(_lookupFunction(ti->hash));
    vals.push_back(_lookupFunction(ti->equal));
    vals.push_back(_lookupFunction(ti->blockable));
    vals.push_back(_lookupFunction(ti->dtor));

    if ( ptype ) {
        // Add type parameters.
        for ( auto p : ptype->parameters() ) {
            auto pt = dynamic_cast<type::trait::parameter::Type *>(p.get());
            auto pi = dynamic_cast<type::trait::parameter::Integer *>(p.get());

            if ( pt )
                vals.push_back(cg()->llvmRtti(pt->type()));

            else if ( pi )
                vals.push_back(cg()->llvmConstInt(pi->value(), 64));

            else
                internalError("unexpected type parameter type");
        }
    }

    return cg()->llvmConstStruct(vals);
}

void TypeBuilder::visit(type::Any* a)
{
    TypeInfo* ti = new TypeInfo(a);
    ti->id = HLT_TYPE_ANY;
    ti->c_prototype = "void *";
    ti->llvm_type = cg()->llvmTypePtr();
    ti->pass_type_info = true;
    setResult(ti);
}

void TypeBuilder::visit(type::Void* a)
{
    TypeInfo* ti = new TypeInfo(a);
    ti->id = HLT_TYPE_VOID;
    ti->c_prototype = "void";
    ti->llvm_type = cg()->llvmTypeVoid();
    setResult(ti);
}

void TypeBuilder::visit(type::String* s)
{
    TypeInfo* ti = new TypeInfo(s);
    ti->id = HLT_TYPE_STRING;
    ti->c_prototype = "hlt_string";
    ti->init_val = cg()->llvmConstNull(cg()->llvmTypeString());
    ti->to_string = "hlt::string_to_string";
    // ti->hash = "hlt::string_hash";
    // ti->equal = "hlt::string_equal";
    setResult(ti);
}

void TypeBuilder::visit(type::Bool* b)
{
    TypeInfo* ti = new TypeInfo(b);
    ti->id = HLT_TYPE_BOOL;
    ti->c_prototype = "int8_t";
    ti->init_val = cg()->llvmConstInt(0, 1);
    ti->to_string = "hlt::bool_to_string";
    ti->to_int64 = "hlt::bool_to_int64";
    // ti->hash = "hlt::bool_hash";
    // ti->equal = "hlt::bool_equal";
    setResult(ti);
}

void TypeBuilder::visit(type::Bytes* b)
{
    TypeInfo* ti = new TypeInfo(b);
    ti->id = HLT_TYPE_BYTES;
    ti->c_prototype = "hlt_bytes*";
    ti->init_val = cg()->llvmConstNull(cg()->llvmLibType("hlt.bytes"));
    ti->to_string = "hlt::bytes_to_string";
    // ti->hash = "hlt::bool_hash";
    // ti->equal = "hlt::bool_equal";
    setResult(ti);
}

void TypeBuilder::visit(type::Integer* i)
{
    TypeInfo* ti = new TypeInfo(i);
    ti->id = HLT_TYPE_INTEGER;
    ti->init_val = cg()->llvmConstInt(0, i->width());
    ti->to_string = "hlt::int_to_string";
    ti->to_int64 = "hlt::int_to_int64";
    // ti->hash = "hlt::default_hash";
    // ti->equal = "hlt::default_equal";

    if ( i->width() <= 8 )
        ti->c_prototype = "int8_t";
    else if ( i->width() <= 16 )
        ti->c_prototype = "int16_t";
    else if ( i->width() <= 32 )
        ti->c_prototype = "int16_t";
    else if ( i->width() <= 64 )
        ti->c_prototype = "int16_t";
    else
        assert(false);

    setResult(ti);
}

void TypeBuilder::visit(type::Tuple* t)
{
    // The default value for tuples sets all elements to their default.
    std::vector<llvm::Constant*> elems;

    for ( auto t : t->typeList() )
        elems.push_back(cg()->llvmInitVal(t));

    auto init_val = cg()->llvmConstStruct(elems);

    // Aux information is an array with the fields' offsets. We calculate the
    // offsets via a sizeof-style hack ...
    std::vector<llvm::Constant*> offsets;
    auto null = cg()->llvmConstNull(cg()->llvmTypePtr(init_val->getType()));
    auto zero = cg()->llvmGEPIdx(0);

    for ( int i = 0; i < t->typeList().size(); ++i ) {
        auto offset = cg()->llvmGEP(null, zero, cg()->llvmGEPIdx(i));
        offsets.push_back(llvm::ConstantExpr::getPtrToInt(offset, cg()->llvmTypeInt(16)));
    }

    llvm::Constant* aux;
    if ( offsets.size() ) {
        auto cval = cg()->llvmConstArray(offsets);
        aux = cg()->llvmAddConst("offsets", cval);
    }

    else
        aux = cg()->llvmConstNull();

    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_TUPLE;
    ti->ptr_map = PointerMap(cg(), t).llvmMap();
    ti->c_prototype = "hlt_tuple";
    ti->init_val = init_val;
    ti->pass_type_info = t->wildcard();
    ti->to_string = "hlt::tuple_to_string";
    // ti->hash = "hlt::tuple_hash";
    // ti->equal = "hlt::tuple_equal";
    ti->aux = aux;

    setResult(ti);
}

void TypeBuilder::visit(type::Reference* b)
{
    shared_ptr<hilti::Type> t = nullptr;

    if ( ! b->wildcard() ) {
        setResult(typeInfo(b->refType()).get());
        return;
    }

    // ref<*>
    TypeInfo* ti = new TypeInfo(b);
    ti->id = HLT_TYPE_ANY;
    ti->init_val = cg()->llvmConstNull();
    ti->c_prototype = "void*";
    setResult(ti);
}
