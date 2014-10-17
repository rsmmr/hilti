
#include <algorithm>

#include "../hilti.h"

#include "type-builder.h"
#include "codegen.h"

#include "../../libhilti/rtti.h"
#include "../../libhilti/port.h"
#include "../../libhilti/enum.h"

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

TypeBuilder::TypeBuilder(CodeGen* cg) : CGVisitor<TypeInfo *, bool>(cg, "codegen::TypeBuilder")
{
}

TypeBuilder::~TypeBuilder()
{
}

void TypeBuilder::finalize()
{
    for ( auto dtor : _struct_dtors )
        _makeStructDtor(dtor.first, dtor.second);
}

TypeInfo* TypeBuilder::typeInfo(shared_ptr<hilti::Type> type, bool llvm_type_only)
{
    if ( ! llvm_type_only )
        typeInfo(type, true);

    auto i = _ti_cache.find(type);

    if ( i != _ti_cache.end() )
        // Already computed.
        return i->second;

    setArg1(llvm_type_only);
    setDefaultResult(nullptr);

    TypeInfo* ti;
    bool success = processOne(type, &ti);
    assert(success);

#ifdef DEBUG
    if ( ! ti ) {
        fprintf(stderr, "No type info for %s\n", type->render().c_str());
        abort();
    }
#endif

    if ( ! ti->init_val && ti->lib_type.size() )
        ti->init_val = cg()->llvmConstNull(cg()->llvmTypePtr(cg()->llvmLibType(ti->lib_type)));

    if ( ! ti->llvm_type && ti->lib_type.size() ) {
        ti->llvm_type = cg()->llvmLibType(ti->lib_type);

        if ( type::hasTrait<type::trait::HeapType>(type) )
            ti->llvm_type = cg()->llvmTypePtr(ti->llvm_type);
    }

    if ( type::hasTrait<type::trait::HeapType>(type) ) {
        ti->obj_dtor = ti->dtor;
        ti->obj_dtor_func = ti->dtor_func;
        ti->cctor_func = cg()->llvmLibFunction("__hlt_object_ref");
        ti->dtor_func = cg()->llvmLibFunction("__hlt_object_unref");
    }

    if ( type::hasTrait<type::trait::GarbageCollected>(type) ) {
        if ( ! ti->cctor_func )
            ti->cctor_func = cg()->llvmLibFunction("__hlt_object_ref");

        if ( ! ti->dtor_func )
            ti->dtor_func = cg()->llvmLibFunction("__hlt_object_unref");
    }

    if ( type::hasTrait<type::trait::Hashable>(type) &&
         type::hasTrait<type::trait::ValueType>(type) ) {
        if ( ti->hash == "" )
            ti->hash = "hlt::default_hash";

        if ( ti->equal == "" )
            ti->equal = "hlt::default_equal";
    }

    if ( ! ti->name.size() )
        ti->name = type->render();

    if ( ti->llvm_type && ! ti->init_val && type::hasTrait<type::trait::ValueType>(type) )
        ti->init_val = cg()->llvmConstNull(ti->llvm_type);

    if ( ! ti )
        internalError(::util::fmt("typeInfo() not available for type '%s'", type->render().c_str()));

    if ( ! ti->llvm_type && ti->init_val )
        ti->llvm_type = ti->init_val->getType();

    if ( ti->id == HLT_TYPE_ERROR )
        internalError(::util::fmt("type info for type %s does not set an RTTI id", type->render().c_str()));

    if ( ast::isA<type::HiltiType>(type) && ! ti->llvm_type )
        internalError(::util::fmt("type info for %s does not define an llvm_type", ti->name.c_str()));

    if ( type::hasTrait<type::trait::ValueType>(type) && ! ti->init_val )
        internalError(::util::fmt("type info for %s does not define an init value", ti->name.c_str()));

#if 0
    if ( type::hasTrait<type::trait::GarbageCollected>(type) && ! ti->ptr_map )
        internalError(::util::fmt("type info for %s does not define a ptr map", ti->name.c_str()));

    if ( type::hasTrait<type::trait::GarbageCollected>(type) && ! ti->dtor )
        internalError(::util::fmt("type info for %s does not define a dtor function", ti->name.c_str()));
#endif

    if ( ! llvm_type_only )
        _ti_cache.insert(make_tuple(type, ti));

    return ti;
}

llvm::Type* TypeBuilder::llvmType(shared_ptr<Type> type)
{
    auto rtype = ast::as<type::Reference>(type);

    if ( rtype && ! rtype->wildcard() )
        // For references, "unwrap" directly here to avoid recursion trouble
        // with cyclic types.
        type = rtype->argType();

    auto ti = typeInfo(type, true);
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

    int gc = 0;
    int atomic = 0;

    auto rt = ast::as<type::Reference>(type);

    if ( type::hasTrait<type::trait::GarbageCollected>(type)
         || (rt && type::hasTrait<type::trait::GarbageCollected>(rt->argType())) )
        gc = 1;

    if ( type::hasTrait<type::trait::Atomic>(type) )
        atomic = 1;

    auto ti = typeInfo(type);

    llvm::Constant* sizeof_ = nullptr;

    if ( ti->init_val )
        sizeof_ = llvm::ConstantExpr::getTrunc(cg()->llvmSizeOf(ti->init_val), cg()->llvmTypeInt(16));
    else
        sizeof_ = cg()->llvmConstInt(0, 16);

    llvm::Constant* object_size = nullptr;

    if ( gc && ti->object_type )
        object_size = llvm::ConstantExpr::getTrunc(cg()->llvmSizeOf(ti->object_type), cg()->llvmTypeInt(16));
    else
        object_size = cg()->llvmConstInt(0, 16);

    if ( ti->atomic && (ti->clone_init.size() || ti->clone_init_func || ti->clone_alloc.size() || ti->clone_alloc_func) )
        internalError(::util::fmt("clone function(s) specified for atomic type '%s'", type->render().c_str()));

    if ( ti->atomic && (ti->obj_dtor.size() || ti->obj_dtor_func || ti->cctor_func || ti->dtor_func) )
        internalError(::util::fmt("ctor or dtor function(s) specified for atomic type '%s'", type->render().c_str()));

    // Ok if not the right type here.
    shared_ptr<type::trait::Parameterized> ptype = std::dynamic_pointer_cast<type::trait::Parameterized>(ti->type);

    int num_params = 0;

    if ( ptype )
        num_params = ptype->parameters().size();

    vals.push_back(cg()->llvmConstInt(ti->id, 16));
    vals.push_back(sizeof_);
    vals.push_back(object_size);
    vals.push_back(cg()->llvmConstAsciizPtr(ti->name));
    vals.push_back(cg()->llvmConstInt(num_params, 16));
    vals.push_back(cg()->llvmConstInt(gc, 16));
    vals.push_back(cg()->llvmConstInt(atomic, 16));
    vals.push_back(ti->aux ? ti->aux : cg()->llvmConstNull(cg()->llvmTypePtr()));
    vals.push_back(ti->ptr_map ? ti->ptr_map : cg()->llvmConstNull(cg()->llvmTypePtr()));
    vals.push_back(_lookupFunction(ti->to_string));
    vals.push_back(_lookupFunction(ti->to_int64));
    vals.push_back(_lookupFunction(ti->to_double));
    vals.push_back(_lookupFunction(ti->hash));
    vals.push_back(_lookupFunction(ti->equal));
    vals.push_back(_lookupFunction(ti->blockable));
    vals.push_back(ti->dtor_func ? ti->dtor_func : _lookupFunction(ti->dtor));
    vals.push_back(ti->obj_dtor_func ? ti->obj_dtor_func : _lookupFunction(ti->obj_dtor));
    vals.push_back(ti->cctor_func ? ti->cctor_func : _lookupFunction(ti->cctor));
    vals.push_back(ti->clone_init_func ? ti->clone_init_func : _lookupFunction(ti->clone_init));
    vals.push_back(ti->clone_alloc_func ? ti->clone_alloc_func : _lookupFunction(ti->clone_alloc));

    if ( ptype ) {
        // Add type parameters.
        for ( auto p : ptype->parameters() ) {
            auto pt = dynamic_cast<type::trait::parameter::Type *>(p.get());
            auto pi = dynamic_cast<type::trait::parameter::Integer *>(p.get());
            auto pe = dynamic_cast<type::trait::parameter::Enum *>(p.get());
            auto pa = dynamic_cast<type::trait::parameter::Attribute *>(p.get());

            if ( pt ) {
                if ( ! _rtti_type_only )
                    vals.push_back(cg()->llvmRtti(pt->type()));
                else
                    vals.push_back(cg()->llvmConstNull(cg()->llvmTypePtr(cg()->llvmTypeRtti())));
            }

            else if ( pi )
                vals.push_back(cg()->llvmConstInt(pi->value(), 64));

            else if ( pe ) {
                auto expr = cg()->hiltiModule()->body()->scope()->lookupUnique(pe->label());
                assert(expr);
                auto val = ast::as<type::Enum>(expr->type())->labelValue(pe->label());
                vals.push_back(cg()->llvmConstInt(val, 64));
            }

            else if ( pa )
                vals.push_back(cg()->llvmConstAsciizPtr(pa->value()));

            else
                internalError("unexpected type parameter type");
        }
    }

    return cg()->llvmConstStruct(vals);
}

llvm::Type* TypeBuilder::llvmRttiType(shared_ptr<hilti::Type> type)
{
    _rtti_type_only = true;
    auto rtti = llvmRtti(type);
    _rtti_type_only = false;
    return rtti->getType();
}

void TypeBuilder::visit(type::Any* a)
{
    TypeInfo* ti = new TypeInfo(a);
    ti->id = HLT_TYPE_ANY;
    ti->llvm_type = cg()->llvmTypePtr();
    ti->pass_type_info = true;
    setResult(ti);
}

void TypeBuilder::visit(type::Void* a)
{
    TypeInfo* ti = new TypeInfo(a);
    ti->id = HLT_TYPE_VOID;
    ti->llvm_type = cg()->llvmTypeVoid();
    setResult(ti);
}

void TypeBuilder::visit(type::String* s)
{
    TypeInfo* ti = new TypeInfo(s);
    ti->id = HLT_TYPE_STRING;
    ti->obj_dtor = "hlt::string_dtor";
    ti->cctor_func = cg()->llvmLibFunction("__hlt_object_ref");
    ti->dtor_func = cg()->llvmLibFunction("__hlt_object_unref");
    ti->init_val = cg()->llvmConstNull(cg()->llvmTypeString());
    ti->to_string = "hlt::string_to_string";
    ti->hash = "hlt::string_hash";
    ti->equal = "hlt::string_equal";
    ti->clone_alloc = "hlt::string_clone_alloc";
    ti->clone_init = "hlt::string_clone_init";
    setResult(ti);
}

void TypeBuilder::visit(type::Bool* b)
{
    TypeInfo* ti = new TypeInfo(b);
    ti->id = HLT_TYPE_BOOL;
    ti->init_val = cg()->llvmConstInt(0, 1);
    ti->to_string = "hlt::bool_to_string";
    ti->to_int64 = "hlt::bool_to_int64";
    setResult(ti);
}

void TypeBuilder::visit(type::Exception* e)
{
    TypeInfo* ti = new TypeInfo(e);
    ti->id = HLT_TYPE_EXCEPTION;
    ti->dtor = "hlt::exception_dtor";
    ti->lib_type = "hlt.exception";

    // Aux points to the exception's type object.
    // ti->aux = cg()->llvmExceptionTypeObject(e->sharedPtr<type::Exception>());

    // ti->to_string = "hlt::exception_to_string";
    setResult(ti);
}

void TypeBuilder::visit(type::iterator::Bytes* i)
{
    TypeInfo* ti = new TypeInfo(i);
    ti->id = HLT_TYPE_ITERATOR_BYTES;
    ti->dtor = "hlt::iterator_bytes_dtor";
    ti->cctor = "hlt::iterator_bytes_cctor";
    ti->clone_init = "hlt::iterator_bytes_clone_init";
    ti->init_val = cg()->llvmConstNull(cg()->llvmLibType("hlt.iterator.bytes"));
    setResult(ti);
}

void TypeBuilder::visit(type::iterator::List* i)
{
    TypeInfo* ti = new TypeInfo(i);
    ti->id = HLT_TYPE_ITERATOR_LIST;
    ti->dtor = "hlt::iterator_list_dtor";
    ti->cctor = "hlt::iterator_list_cctor";
    ti->init_val = cg()->llvmConstNull(cg()->llvmLibType("hlt.iterator.list"));
    setResult(ti);
}

void TypeBuilder::visit(type::iterator::Set* i)
{
    TypeInfo* ti = new TypeInfo(i);
    ti->id = HLT_TYPE_ITERATOR_SET;
    ti->dtor = "hlt::iterator_set_dtor";
    ti->cctor = "hlt::iterator_set_cctor";
    ti->init_val = cg()->llvmConstNull(cg()->llvmLibType("hlt.iterator.set"));
    setResult(ti);
}

void TypeBuilder::visit(type::iterator::Map* i)
{
    TypeInfo* ti = new TypeInfo(i);
    ti->id = HLT_TYPE_ITERATOR_MAP;
    ti->dtor = "hlt::iterator_map_dtor";
    ti->cctor = "hlt::iterator_map_cctor";
    ti->init_val = cg()->llvmConstNull(cg()->llvmLibType("hlt.iterator.map"));
    setResult(ti);
}

void TypeBuilder::visit(type::iterator::Vector* i)
{
    TypeInfo* ti = new TypeInfo(i);
    ti->id = HLT_TYPE_ITERATOR_VECTOR;
    ti->dtor = "hlt::iterator_vector_dtor";
    ti->cctor = "hlt::iterator_vector_cctor";
    ti->init_val = cg()->llvmConstNull(cg()->llvmLibType("hlt.iterator.vector"));
    setResult(ti);
}

void TypeBuilder::visit(type::iterator::IOSource* i)
{
    // If we just lookup hlt.iterator.iosrc, we get type conflicts due to
    // LLVM's type unification. So we build it manually.
    auto src_type = cg()->llvmTypePtr(cg()->llvmLibType("hlt.iosrc"));
    CodeGen::type_list iter_fields = { src_type, cg()->llvmTypeInt(64), cg()->llvmTypePtr(cg()->llvmLibType("hlt.bytes")) };
    auto iter_type = cg()->llvmTypeStruct("", iter_fields);

    TypeInfo* ti = new TypeInfo(i);
    ti->id = HLT_TYPE_ITERATOR_IOSRC;
    ti->dtor = "hlt::iterator_iosrc_dtor";
    ti->cctor = "hlt::iterator_iosrc_cctor";
    ti->init_val = cg()->llvmConstNull(iter_type);
    setResult(ti);
}

void TypeBuilder::visit(type::Integer* i)
{
    TypeInfo* ti = new TypeInfo(i);
    ti->id = HLT_TYPE_INTEGER;
    ti->init_val = cg()->llvmConstInt(0, i->width());
    ti->to_string = "hlt::int_to_string";
    ti->to_int64 = "hlt::int_to_int64";

    setResult(ti);
}

llvm::Function* TypeBuilder::_makeTupleDtor(CodeGen* cg, type::Tuple* t)
{
    // Creates the destructor for tuples.
    return _makeTupleFuncHelper(cg, t, true);
}

llvm::Function* TypeBuilder::_makeTupleCctor(CodeGen* cg, type::Tuple* t)
{
    // Creates the cctor for tuples.
    return _makeTupleFuncHelper(cg, t, false);
}

llvm::Function* TypeBuilder::_makeTupleFuncHelper(CodeGen* cg, type::Tuple* t, bool dtor)
{
    auto type = t->sharedPtr<Type>();

    string prefix = dtor ? "dtor_" : "cctor_";
    string name = prefix + type->render();

    llvm::Value* cached = cg->lookupCachedValue(prefix + "-tuple", name);

    if ( cached )
        return llvm::cast<llvm::Function>(cached);

    // Build tuple type. Can't use llvmType() as that would recurse.
    std::vector<llvm::Type*> fields;
    for ( auto t : t->typeList() )
        fields.push_back(cg->llvmType(t));

    auto llvm_type = cg->llvmTypeStruct("", fields);

    CodeGen::llvm_parameter_list params;
    params.push_back(std::make_pair("type", cg->llvmTypePtr(cg->llvmTypeRtti())));
    params.push_back(std::make_pair("tuple", cg->llvmTypePtr(llvm_type)));
    params.push_back(std::make_pair(codegen::symbols::ArgExecutionContext, cg->llvmTypePtr(cg->llvmTypeExecutionContext())));

    auto func = cg->llvmAddFunction(name, cg->llvmTypeVoid(), params, false);
    func->setLinkage(llvm::GlobalValue::LinkOnceAnyLinkage);

    cg->pushFunction(func);

    auto a = func->arg_begin();
    ++a;
    auto tval = cg->builder()->CreateLoad(a);

    int idx = 0;

    for ( auto et : t->typeList() ) {
        auto elem = cg->llvmExtractValue(tval, idx++);

        if ( dtor )
            cg->llvmDtor(elem, et, false, "tuple");
        else
            cg->llvmCctor(elem, et, false, "tuple");
    }

    cg->popFunction();

    cg->cacheValue(prefix + "-tuple", name, func);

    return func;
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
    // ti->ptr_map = PointerMap(cg(), t).llvmMap();
    ti->init_val = init_val;
    ti->pass_type_info = t->wildcard();
    ti->to_string = "hlt::tuple_to_string";
    ti->hash = "hlt::tuple_hash";
    ti->equal = "hlt::tuple_equal";

    // TODO: Is is worth it to generate a per-type function here for
    // non-wildcard tuples?
    ti->clone_init = "hlt::tuple_clone_init";

    ti->aux = aux;

    if ( ! t->wildcard() ) {
        ti->dtor_func = _makeTupleDtor(cg(), t);
        ti->cctor_func = _makeTupleCctor(cg(), t);
    }
    else {
        // Generic versions working with all tuples.
        ti->dtor = "hlt::tuple_dtor";
        ti->cctor = "hlt::tuple_cctor";
    }

    setResult(ti);
}

void TypeBuilder::visit(type::Reference* b)
{
    shared_ptr<hilti::Type> t = nullptr;

    if ( ! b->wildcard() ) {
        auto ti = typeInfo(b->argType());
        setResult(ti);
        return;
    }

    // ref<*>
    TypeInfo* ti = new TypeInfo(b);
    ti->id = HLT_TYPE_ANY;
    ti->init_val = cg()->llvmConstNull();
    setResult(ti);
}

void TypeBuilder::visit(type::Address* t)
{
    CodeGen::constant_list default_;
    default_.push_back(cg()->llvmConstInt(0, 64));
    default_.push_back(cg()->llvmConstInt(0, 64));

    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_ADDR;
    ti->init_val = cg()->llvmConstStruct(default_);
    ti->to_string = "hlt::addr_to_string";
    setResult(ti);
}

void TypeBuilder::visit(type::Network* t)
{
    CodeGen::constant_list default_;
    default_.push_back(cg()->llvmConstInt(0, 64));
    default_.push_back(cg()->llvmConstInt(0, 64));
    default_.push_back(cg()->llvmConstInt(0, 8));

    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_NET;
    ti->init_val = cg()->llvmConstStruct(default_);
    ti->to_string = "hlt::net_to_string";
    setResult(ti);
}

void TypeBuilder::visit(type::CAddr* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_CADDR;
    ti->to_string = "hlt::caddr_to_string";
    ti->init_val = cg()->llvmConstNull(cg()->llvmTypePtr());
    setResult(ti);
}

void TypeBuilder::visit(type::Double* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_DOUBLE;
    ti->init_val = cg()->llvmConstDouble(0);
    ti->to_string = "hlt::double_to_string";
    ti->to_double = "hlt::double_to_double";
    setResult(ti);
}

void TypeBuilder::visit(type::Bitset* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_BITSET;
    ti->init_val = cg()->llvmConstInt(0, 64);
    ti->to_string = "hlt::bitset_to_string";
    ti->to_int64 = "hlt::bitset_to_int64";

    // Aux information is a list of tuple <uint8_t, const char*>.
    CodeGen::constant_list tuples;

    for ( auto l : t->labels() ) {
        auto label = cg()->llvmConstAsciizPtr(l.first->pathAsString());
        auto bit = cg()->llvmConstInt(l.second, 8);

        CodeGen::constant_list elems;
        elems.push_back(bit);
        elems.push_back(label);
        tuples.push_back(cg()->llvmConstStruct(elems));
    }

    // End marker.
    CodeGen::constant_list elems;
    elems.push_back(cg()->llvmConstInt(0, 8));
    elems.push_back(cg()->llvmConstNull());
    tuples.push_back(cg()->llvmConstStruct(elems));

    auto aux = cg()->llvmConstArray(tuples);
    auto glob = cg()->llvmAddGlobal("bitset_labels", aux->getType(), aux);
    glob->setLinkage(llvm::GlobalValue::LinkOnceAnyLinkage);

    ti->aux = glob;

    setResult(ti);
}

void TypeBuilder::visit(type::Enum* t)
{
    CodeGen::constant_list default_;
    default_.push_back(cg()->llvmConstInt(HLT_ENUM_UNDEF, 64));
    default_.push_back(cg()->llvmConstInt(0, 64));

    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_ENUM;
    ti->init_val = cg()->llvmConstStruct(default_, true);
    ti->to_string = "hlt::enum_to_string";
    ti->to_int64 = "hlt::enum_to_int64";

    // Aux information is a list of tuple <uint64_t, const char*>.
    CodeGen::constant_list tuples;

    for ( auto l : t->labels() ) {
        auto label = cg()->llvmConstAsciizPtr(l.first->pathAsString());
        auto bit = cg()->llvmConstInt(l.second, 64);

        CodeGen::constant_list elems;
        elems.push_back(bit);
        elems.push_back(label);
        tuples.push_back(cg()->llvmConstStruct(elems));
    }

    // End marker.
    CodeGen::constant_list elems;
    elems.push_back(cg()->llvmConstInt(0, 64));
    elems.push_back(cg()->llvmConstNull());
    tuples.push_back(cg()->llvmConstStruct(elems));

    auto aux = cg()->llvmConstArray(tuples);
    auto glob = cg()->llvmAddGlobal("enum_labels", aux->getType(), aux);
    glob->setLinkage(llvm::GlobalValue::LinkOnceAnyLinkage);

    ti->aux = glob;

    setResult(ti);
}

void TypeBuilder::visit(type::Interval* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_INTERVAL;
    ti->init_val = cg()->llvmConstInt(0, 64);
    ti->to_string = "hlt::interval_to_string";
    ti->to_double = "hlt::interval_to_double";
    ti->to_int64 = "hlt::interval_to_int64";
    setResult(ti);}

void TypeBuilder::visit(type::Time* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_TIME;
    ti->init_val = cg()->llvmConstInt(0, 64);
    ti->to_string = "hlt::time_to_string";
    ti->to_double = "hlt::time_to_double";
    ti->to_int64 = "hlt::time_to_int64";
    setResult(ti);
}

void TypeBuilder::visit(type::Port* t)
{
    CodeGen::constant_list default_;
    default_.push_back(cg()->llvmConstInt(0, 16));
    default_.push_back(cg()->llvmConstInt(HLT_PORT_TCP, 8));

    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_PORT;
    ti->init_val = cg()->llvmConstStruct(default_, true);
    ti->to_string = "hlt::port_to_string";
    ti->to_int64 = "hlt::port_to_int64";

    setResult(ti);
}

void TypeBuilder::visit(type::Bytes* b)
{
    TypeInfo* ti = new TypeInfo(b);
    ti->id = HLT_TYPE_BYTES;
    ti->dtor = "hlt::bytes_dtor";
    ti->lib_type = "hlt.bytes";
    ti->to_string = "hlt::bytes_to_string";
    ti->hash = "hlt::bytes_hash";
    ti->equal = "hlt::bytes_equal";
    ti->blockable = "hlt::bytes_blockable";
    ti->clone_alloc = "hlt::bytes_clone_alloc";
    ti->clone_init = "hlt::bytes_clone_init";
    setResult(ti);
}

void TypeBuilder::visit(type::Callable* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_CALLABLE;
    ti->dtor = "hlt::callable_dtor";
    ti->lib_type = "hlt.callable";
    ti->clone_alloc = "hlt::callable_clone_alloc";
    ti->clone_init = "hlt::callable_clone_init";
    setResult(ti);
}

void TypeBuilder::visit(type::Channel* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_CHANNEL;
    ti->dtor = "hlt::channel_dtor";
    ti->lib_type = "hlt.channel";
    ti->to_string = "hlt::channel_to_string";
    ti->clone_alloc = "hlt::channel_clone_alloc";
    ti->clone_init = "hlt::channel_clone_init";
    setResult(ti);
}

void TypeBuilder::visit(type::Classifier* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_CLASSIFIER;
    ti->dtor = "hlt::classifier_dtor";
    ti->lib_type = "hlt.classifier";
    setResult(ti);
}

void TypeBuilder::visit(type::File* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_FILE;
    ti->dtor = "hlt::file_dtor";
    ti->lib_type = "hlt.file";
    ti->to_string = "hlt::file_to_string";
    ti->clone_alloc = "hlt::file_clone_alloc";
    ti->clone_init = "hlt::file_clone_init";
    setResult(ti);
}

void TypeBuilder::visit(type::Function* t)
{
    auto ftype = cg()->llvmFunctionType(t->sharedPtr<type::Function>());

    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_FUNCTION;
    ti->init_val = cg()->llvmConstNull(cg()->llvmTypePtr(ftype));
    setResult(ti);
}

void TypeBuilder::visit(type::IOSource* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_IOSOURCE;
    ti->dtor = "hlt::iosrc_dtor";
    ti->lib_type = "hlt.iosrc";
    setResult(ti);
}

void TypeBuilder::visit(type::List* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_LIST;
    ti->dtor = "hlt::list_dtor";
    ti->lib_type = "hlt.list";
    ti->to_string = "hlt::list_to_string";
    ti->clone_alloc = "hlt::list_clone_alloc";
    ti->clone_init = "hlt::list_clone_init";

    setResult(ti);
}

void TypeBuilder::visit(type::Map* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_MAP;
    ti->dtor = "hlt::map_dtor";
    ti->lib_type = "hlt.map";
    ti->to_string = "hlt::map_to_string";
    ti->clone_alloc = "hlt::map_clone_alloc";
    ti->clone_init = "hlt::map_clone_init";
    setResult(ti);
}

void TypeBuilder::visit(type::Vector* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_VECTOR;
    ti->dtor = "hlt::vector_dtor";
    ti->lib_type = "hlt.vector";
    ti->to_string = "hlt::vector_to_string";
    ti->clone_alloc = "hlt::vector_clone_alloc";
    ti->clone_init = "hlt::vector_clone_init";
    setResult(ti);
}

void TypeBuilder::visit(type::Set* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_SET;
    ti->dtor = "hlt::set_dtor";
    ti->lib_type = "hlt.set";
    ti->to_string = "hlt::set_to_string";
    ti->clone_alloc = "hlt::set_clone_alloc";
    ti->clone_init = "hlt::set_clone_init";
    setResult(ti);
}

llvm::Function* TypeBuilder::_makeOverlayCctor(CodeGen* cg, type::Overlay* t, llvm::Type* llvm_type)
{
    return _makeOverlayFuncHelper(cg, t, llvm_type, false);
}

llvm::Function* TypeBuilder::_makeOverlayDtor(CodeGen* cg, type::Overlay* t, llvm::Type* llvm_type)
{
    return _makeOverlayFuncHelper(cg, t, llvm_type, true);
}

llvm::Function* TypeBuilder::_makeOverlayFuncHelper(CodeGen* cg, type::Overlay* t, llvm::Type* llvm_type, bool dtor)
{
    string prefix = dtor ? "dtor_" : "cctor_";
    string name = prefix + ::util::fmt("dtor_overlay_dep%d", t->numDependencies());

    llvm::Value* cached = cg->lookupCachedValue(prefix + "-overlay", name);

    if ( cached )
        return llvm::cast<llvm::Function>(cached);

    CodeGen::llvm_parameter_list params;
    params.push_back(std::make_pair("type", cg->llvmTypePtr(cg->llvmTypeRtti())));
    params.push_back(std::make_pair("overlay", cg->llvmTypePtr(llvm_type)));
    params.push_back(std::make_pair(codegen::symbols::ArgExecutionContext, cg->llvmTypePtr(cg->llvmTypeExecutionContext())));

    auto func = cg->llvmAddFunction(name, cg->llvmTypeVoid(), params, false);

    cg->pushFunction(func);

    auto itype = builder::iterator::type(builder::bytes::type());
    auto oval = cg->builder()->CreateLoad(++func->arg_begin());

    for ( int i = 0; i < 1 + t->numDependencies(); ++i) {
        auto iter = cg->llvmExtractValue(oval, i);

        if ( dtor )
            cg->llvmDtor(iter, itype, false, "overlay-dtor");
        else
            cg->llvmCctor(iter, itype, false, "overlay-cctor");
    }

    cg->popFunction();

    cg->cacheValue(prefix + "-overlay", name, func);

    return func;
}

void TypeBuilder::visit(type::Overlay* t)
{
    auto itype = cg()->llvmType(builder::iterator::type(builder::bytes::type()));
    auto atype = llvm::ArrayType::get(itype, 1 + t->numDependencies());

    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_OVERLAY;
    ti->dtor_func = _makeOverlayDtor(cg(), t, atype);
    ti->init_val = cg()->llvmConstNull(atype);
    setResult(ti);
}

void TypeBuilder::visit(type::RegExp* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_REGEXP;
    ti->dtor = "hlt::regexp_dtor";
    ti->lib_type = "hlt.regexp";
    ti->to_string = "hlt::regexp_to_string";
    ti->clone_alloc = "hlt::regexp_clone_alloc";
    ti->clone_init = "hlt::regexp_clone_init";

    // The type-specific aux information is a single int64 corresponding to
    // hlt_regexp_flags.

    int64_t flags = 0;

    if ( t->attributes().has(attribute::NOSUB) )
        flags |= 1;

    ti->aux = llvm::ConstantExpr::getIntToPtr(cg()->llvmConstInt(flags, 64), cg()->llvmTypePtr());

    setResult(ti);
}

void TypeBuilder::visit(type::MatchTokenState* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_MATCH_TOKEN_STATE;
    ti->dtor = "hlt::match_token_state_dtor";
    ti->lib_type = "hlt.match_token_state";
    setResult(ti);
}

// This function just declares the function, we build the implementation at
// the end when all type information has been generated. Otherwise we'd run
// into trouble with cyclic structures as they function already needs the
// fields' type information already.
llvm::Function* TypeBuilder::_declareStructDtor(type::Struct* t, llvm::Type* llvm_type, const string& external_name)
{
    auto type = t->sharedPtr<Type>();

    string name = external_name.size() ? external_name : "dtor_" + type->render();

    llvm::Value* cached = cg()->lookupCachedValue("dtor-struct", name);

    if ( cached )
        return llvm::cast<llvm::Function>(cached);

    CodeGen::llvm_parameter_list params;
    params.push_back(std::make_pair("type", cg()->llvmTypePtr(cg()->llvmTypeRtti())));
    params.push_back(std::make_pair("struct", cg()->llvmTypePtr(llvm_type)));
    params.push_back(std::make_pair(codegen::symbols::ArgExecutionContext, cg()->llvmTypePtr(cg()->llvmTypeExecutionContext())));

    auto func = cg()->llvmAddFunction(name, cg()->llvmTypeVoid(), params, false);
    func->setLinkage(llvm::GlobalValue::LinkOnceODRLinkage);

    if ( external_name.size() )
        func->setCallingConv(llvm::CallingConv::C);

    cg()->cacheValue("dtor-struct", name, func);

    _struct_dtors.push_back(std::make_pair(t, func));

    return func;
}

void TypeBuilder::_makeStructDtor(type::Struct* t, llvm::Function* func)
{
    cg()->pushFunction(func);

    auto a = func->arg_begin();
    ++a;
    auto sval = cg()->builder()->CreateLoad(a);
    auto mask = cg()->llvmExtractValue(sval, 1);

    int idx = 0;

    for ( auto et : t->typeList() ) {

        auto bit = cg()->llvmConstInt(1 << idx, 32);
        auto isset = builder()->CreateAnd(bit, mask);

        auto set = cg()->newBuilder("set");
        auto done = cg()->newBuilder("done");

        auto notzero = builder()->CreateICmpNE(isset, cg()->llvmConstInt(0, 32));
        cg()->llvmCreateCondBr(notzero, set, done);

        cg()->pushBuilder(set);
        auto elem = cg()->llvmExtractValue(sval, 2 + idx++); // first two are gc_hdr and mask.
        cg()->llvmDtor(elem, et, false, "struct");
        cg()->llvmCreateBr(done);
        cg()->popBuilder();

        cg()->pushBuilder(done);
    }

    cg()->popFunction();
}

void TypeBuilder::visit(type::Struct* t)
{
    auto llvm_type_only = arg1();

    string sname;

    if ( t->id() ) {
        sname = t->id()->pathAsString();

        if ( ! t->id()->isScoped() ) {
            auto s = t->scope();
            if ( s.size() )
                sname = ::util::fmt("%s::%s", s, sname);
        }
    }

    else
        sname = t->render();

    sname = util::mangle(sname, true);

    llvm::StructType* stype = nullptr;

    // See if we have already created this struct type.
    auto i = _known_structs.find(sname);

    if ( i != _known_structs.end() )
        stype = i->second;

    else {
        /// Need to create the struct type. We first create it empty and
        /// cache it so that recursion works. We'll then add the right
        ///
        /// fields.
        stype = llvm::StructType::create(cg()->llvmContext(), sname); // opaque type
        _known_structs.insert(std::make_pair(sname, stype));

        // Now add the fields.
        CodeGen::type_list fields { cg()->llvmLibType("hlt.gchdr"), cg()->llvmTypeInt(32) };

        for ( auto f : t->fields() )
            fields.push_back(cg()->llvmType(f->type()));

        stype->setBody(fields);
    }

    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_STRUCT;
    ti->init_val = cg()->llvmConstNull(cg()->llvmTypePtr(stype));
    ti->object_type = stype;
    ti->to_string = "hlt::struct_to_string";
    ti->hash = "hlt::struct_hash";
    ti->equal = "hlt::struct_equal";

    // TODO: Is is worth it to generate per-type functions here for
    // non-wildcard structs?
    ti->clone_alloc = "hlt::struct_clone_alloc";
    ti->clone_init = "hlt::struct_clone_init";

    setResult(ti);

    if ( ! llvm_type_only ) {
        if ( ! t->wildcard() )
            ti->dtor_func = _declareStructDtor(t, stype, t->attributes().getAsString(attribute::LIBHILTI_DTOR, ""));
        else
            // Generic versions working with all tuples.
            ti->dtor = "hlt::struct_dtor";

        /// Type information for a ``struct`` includes the fields' offsets in the
        /// ``aux`` entry as a concatenation of pairs (ASCIIZ*, offset), where
        /// ASCIIZ is a field's name, and offset its offset in the value. Aux
        /// information is a list of tuple <const char*, int16_t>.

        auto zero = cg()->llvmGEPIdx(0);
        auto null = cg()->llvmConstNull(cg()->llvmTypePtr(stype));

        CodeGen::constant_list array;

        int i = 0;

        for ( auto f : t->fields() ) {
            auto n = f->id()->name();

            if ( f->anonymous() )
                n = string(".") + n;

            auto name = cg()->llvmConstAsciizPtr(n);

            // Calculate the offset.
            auto idx = cg()->llvmGEPIdx(i + 2); // skip the gchdr and bitmask.
            auto offset = cg()->llvmGEP(null, zero, idx);
            offset = llvm::ConstantExpr::getPtrToInt(offset, cg()->llvmTypeInt(16));

            CodeGen::constant_list pair { name, offset };
            array.push_back(cg()->llvmConstStruct(pair));

            ++i;
        }

        llvm::GlobalVariable* glob = nullptr;

        if ( array.size() ) {
            auto aval = cg()->llvmConstArray(array);
            glob = cg()->llvmAddConst("struct-fields", aval);
            glob->setLinkage(llvm::GlobalValue::LinkOnceAnyLinkage);
        }

        ti->aux = glob;
    }
}

void TypeBuilder::visit(type::Timer* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_TIMER;
    ti->dtor = "hlt::timer_dtor";
    ti->lib_type = "hlt.timer";
    ti->to_string = "hlt::timer_to_string";
    ti->to_int64 = "hlt::timer_to_int64";
    ti->to_double = "hlt::timer_to_double";
    setResult(ti);
}

void TypeBuilder::visit(type::TimerMgr* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_TIMER_MGR;
    ti->dtor = "hlt::timer_mgr_dtor";
    ti->lib_type = "hlt.timer_mgr";
    ti->to_string = "hlt::timer_mgr_to_string";
    setResult(ti);
}

void TypeBuilder::visit(type::Unset* t)
{
    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_UNSET;
    ti->init_val = cg()->llvmConstInt(0, 1); // just a dummy.
    setResult(ti);
}

void TypeBuilder::visit(type::Union* t)
{
    auto llvm_type_only = arg1();

    string sname;

    if ( t->id() ) {
        sname = t->id()->pathAsString();

        if ( ! t->id()->isScoped() ) {
            auto s = t->scope();
            if ( s.size() )
                sname = ::util::fmt("%s::%s", s, sname);
        }
    }

    else
        sname = t->render();

    sname = util::mangle(sname, true);

    llvm::StructType* stype = nullptr;
    llvm::Type* data_type = nullptr;

    // See if we have already created this union type.
    auto i = _known_unions.find(sname);

    if ( i != _known_unions.end() ) {
        stype = i->second;
        data_type = stype->getElementType(1);
    }

    else {
        /// Need to create the union type. We first create it empty and
        /// cache it so that recursion works. We'll then add the right
        /// fields.
        stype = llvm::StructType::create(cg()->llvmContext(), sname); // opaque type
        _known_unions.insert(std::make_pair(sname, stype));

        // Determine the size of the largest field.
        uint64_t max_size = 0;

        for ( auto f : t->fields() ) {
            auto size = cg()->llvmSizeOfForTarget(cg()->llvmType(f->type()));
            max_size = std::max(max_size, size);
        }

        // Now add the fields.
        data_type = llvm::ArrayType::get(cg()->llvmTypeInt(8), max_size);
        CodeGen::type_list fields { cg()->llvmTypeInt(32), data_type };

        for ( auto f : t->fields() )
            fields.push_back(cg()->llvmType(f->type()));

        stype->setBody(fields);
    }

    TypeInfo* ti = new TypeInfo(t);
    ti->id = HLT_TYPE_UNION;
    ti->init_val = cg()->llvmConstStruct({ cg()->llvmConstInt(-1, 32), cg()->llvmConstNull(data_type) });
    ti->object_type = stype;
    ti->to_string = "hlt::union_to_string";
    ti->hash = "hlt::union_hash";
    ti->equal = "hlt::union_equal";
    ti->clone_init = "hlt::union_clone_init";
    ti->cctor = "hlt::union_cctor";
    ti->dtor = "hlt::union_dtor";

    setResult(ti);

    if ( ! llvm_type_only ) {
        /// Type information for a ``union`` includes the fields' types and names
        /// in the ``aux`` entry.

        CodeGen::constant_list array;

        for ( auto f : t->fields() ) {
            auto name = (t->anonymousFields() || f->anonymous())
                ? cg()->llvmConstNull(cg()->llvmTypePtr())
                : cg()->llvmConstAsciizPtr(f->id()->name());

            CodeGen::constant_list pair { cg()->llvmRtti(f->type()), name };
            array.push_back(cg()->llvmConstStruct(pair));
        }

        llvm::GlobalVariable* glob = nullptr;

        if ( array.size() ) {
            auto aval = cg()->llvmConstArray(array);
            glob = cg()->llvmAddConst("union-fields", aval);
        glob->setLinkage(llvm::GlobalValue::LinkOnceAnyLinkage);
        }

        ti->aux = glob;
    }
}
