
#include "type-builder.h"
#include "../type.h"
#include "../module.h"
#include "../attribute.h"
#include "../expression.h"

#include <hilti/builder/builder.h>

using namespace binpac;
using namespace binpac::codegen;

TypeBuilder::TypeBuilder(CodeGen* cg)
    : CGVisitor<TypeInfo>(cg, "TypeBuilder")
{
}

TypeBuilder::~TypeBuilder()
{
}

void TypeBuilder::_addHostType(shared_ptr<::hilti::Type> type, int pac_type, shared_ptr<::hilti::Expression> aux)
{
    auto i = ::hilti::builder::integer::create(pac_type);

    ::hilti::builder::tuple::element_list elems = { i };

    if ( aux )
        elems.push_back(aux);

    auto t = ::hilti::builder::tuple::create(elems);

    type->attributes().add(::hilti::attribute::HOSTAPP_TYPE, t);
}

shared_ptr<::hilti::Type> TypeBuilder::_buildType(shared_ptr<::hilti::Type> type, int pac_type, shared_ptr<::hilti::Expression> aux)
{
#if 0
    auto mbuilder = cg()->moduleBuilder();

    if ( type->wildcard() )
        tag += "Any";

    auto cidx = tag + "|" + type->render();

    if ( aux )
        cidx += "|" + aux->render();

    if ( auto glob = mbuilder->lookupNode("type-builder", cidx) ) {
        // fprintf(stderr, "cidx1 %s\n", cidx.c_str());
        return ast::checkedCast<hilti::Type>(glob);
    }
#endif

    // fprintf(stderr, "cidx2 %s\n", cidx.c_str());

    _addHostType(type, pac_type, aux);
    return type;

#if 0
    auto id = string("__") + tag;
    auto glob = mbuilder->addType(id, type, true);
    mbuilder->cacheNode("type-builder", cidx, glob);
    return glob;
#endif    
}

shared_ptr<hilti::Type> TypeBuilder::hiltiType(shared_ptr<Type> type, id_list* deps)
{
    _deps = deps;

    TypeInfo result;
    bool success = processOne(type, &result);
    assert(success);

    _deps = nullptr;

    return result.hilti_type;
}

shared_ptr<hilti::ID> TypeBuilder::hiltiTypeID(shared_ptr<Type> type)
{
    TypeInfo result;
    bool success = processOne(type, &result);
    assert(success);

    return result.hilti_id;
}

shared_ptr<hilti::Expression> TypeBuilder::hiltiDefault(shared_ptr<Type> type, bool null_on_default, bool can_be_unset)
{
    TypeInfo result;
    bool success = processOne(type, &result);
    assert(success);

    auto val = result.hilti_default ? result.hilti_default
        : (null_on_default ? shared_ptr<hilti::Expression>() : hilti::builder::expression::default_(result.hilti_type));

    if ( result.always_initialize || ! can_be_unset )
        return val;

    else
        return nullptr;
}

shared_ptr<hilti::Expression> TypeBuilder::hiltiAddParseObjectTypeInfo(shared_ptr<Type> unit)
{
    // Build the aux hostapp data to describe the unit items for the runtime.
    // The built expression is a list of tuples, one for each item of the
    // form:
    //
    // type   - HILTI type of the item's value.
    // kind   - The item type as a BINPAC_UNIT_ITEM_* constant.
    // hide   - Boolean that's true if the &hide attribute is present.
    // path   - A tuple of int<64> that describe the access path to the item from the top-level unit.
    //
    // Note that when changing anything here, libbinpac/rtti.c must be
    // adapted accordingly.

    auto u = ast::checkedCast<type::Unit>(unit);

    ::hilti::builder::tuple::element_list items;

    std::set<string> seen;

    for ( auto i : u->flattenedItems() ) {

        if ( i->anonymous() )
            continue;

        if ( i->attributes()->has("transient") )
            continue;

        if ( seen.find(i->id()->name())  != seen.end() )
             continue;

        binpac_unit_item_kind kind = BINPAC_UNIT_ITEM_NONE;

        if ( ast::isA<type::unit::item::Field>(i) )
            kind = BINPAC_UNIT_ITEM_FIELD;

        else if ( ast::isA<type::unit::item::Variable>(i) )
            kind = BINPAC_UNIT_ITEM_VARIABLE;

        else
            // Don't record any other items.
            continue;

        auto path = cg()->hiltiItemComputePath(cg()->hiltiTypeParseObject(u), i);

        ::hilti::builder::tuple::element_list t;
        t.push_back(::hilti::builder::integer::create(kind));
        t.push_back(::hilti::builder::boolean::create(i->attributes()->has("hide")));
        t.push_back(path);

        items.push_back(::hilti::builder::tuple::create(t));

        seen.insert(i->id()->name());
    }

    auto t = hilti::builder::tuple::create(items);
    auto id = ::util::fmt("__%s_fields", cg()->hiltiID(unit->id(), true)->name());
    return cg()->moduleBuilder()->addConstant(id, t->type(), t);
}

void TypeBuilder::visit(type::Address* a)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::address::type();
    setResult(ti);
}

void TypeBuilder::visit(type::Any* a)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::any::type(a->location());
    setResult(ti);
}

void TypeBuilder::visit(type::Bitfield* b)
{
    ::hilti::builder::type_list types;
    ::hilti::builder::id_list names;

    for ( auto b : b->bits() ) {
        auto ft = b->fieldType();
        types.push_back(hiltiType(ft));
        names.push_back(::hilti::builder::id::node(b->id()->name()));
    }

    auto tt = ::hilti::builder::tuple::type(types);
    tt->setNames(names);

    auto t = _buildType(tt, BINPAC_TYPE_BITFIELD);

    TypeInfo ti;
    ti.hilti_type = t;

    setResult(ti);
}

void TypeBuilder::visit(type::Bitset* b)
{
}

void TypeBuilder::visit(type::Block* b)
{
}

void TypeBuilder::visit(type::Bool* b)
{
    TypeInfo ti;

    auto t = hilti::builder::bytes::type(b->location());
    ti.hilti_type = hilti::builder::boolean::type();

    setResult(ti);
}

void TypeBuilder::visit(type::Bytes* b)
{
    TypeInfo ti;

    auto t = hilti::builder::bytes::type();
    auto bt = t;
    ti.hilti_type = hilti::builder::reference::type(bt);
    ti.hilti_default = hilti::builder::bytes::create("", b->location());

    setResult(ti);
}

void TypeBuilder::visit(type::CAddr* c)
{
}

void TypeBuilder::visit(type::Double* d)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::double_::type();
    setResult(ti);
}

void TypeBuilder::visit(type::Enum* e)
{
    TypeInfo ti;

    hilti::builder::enum_::label_list labels;

    for ( auto l : e->labels() ) {
        if ( l.first->name() == "Undef" )
            continue;

        auto id = hilti::builder::id::node(l.first->name(), l.first->location());
        labels.push_back(std::make_pair(id, l.second));
    }

    auto et = hilti::builder::enum_::type(labels, e->location());

    ti.hilti_type = _buildType(et, BINPAC_TYPE_ENUM);
    ti.hilti_default = ti.hilti_type->typeScope()->lookupUnique(::hilti::builder::id::node("Undef"));
    assert(ti.hilti_default);

    auto id = cg() ? cg()->hiltiID(e->id(), true) : nullptr;

    if ( cg() ) {
        assert(id);

        if ( id->isScoped() )
            cg()->hiltiImportType(std::make_shared<ID>(id->pathAsString()), e->sharedPtr<Type>());

        ti.hilti_type->setID(id);
        ti.hilti_type->setScope(id->scope());
    }

    else
        ti.hilti_type->setID(std::make_shared<hilti::ID>(::util::fmt("enum_no_cg_%p", e)));

    if ( id && id->isScoped() && _deps )
        _deps->push_back(id);

    setResult(ti);
}

void TypeBuilder::visit(type::EmbeddedObject* o)
{
    auto etype = hiltiType(o->argType());

    TypeInfo ti;
    ti.hilti_type = _buildType(etype, BINPAC_TYPE_EMBEDDED_OBJECT);
    setResult(ti);
}

void TypeBuilder::visit(type::Exception* e)
{
}

void TypeBuilder::visit(type::File* f)
{
}

void TypeBuilder::visit(type::Function* f)
{
}

void TypeBuilder::visit(type::Hook* h)
{
}

void TypeBuilder::visit(type::Integer* i)
{
#if 0
    auto signed_ = hilti::builder::boolean::create(i->signed_());
    auto tag = ::util::fmt("Integer%d", i->width());

    TypeInfo ti;
    ti.hilti_type = _buildType(tag, hilti::builder::integer::type(i->width()), BINPAC_TYPE_INTEGER, signed_);
    setResult(ti);
#else
    auto signed_ = i->signed_() ? BINPAC_TYPE_INTEGER_SIGNED : BINPAC_TYPE_INTEGER_UNSIGNED;

    TypeInfo ti;
    ti.hilti_type = _buildType(hilti::builder::integer::type(i->width()), signed_);
    setResult(ti);
#endif
}

void TypeBuilder::visit(type::Interval* i)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::interval::type();
    setResult(ti);
}

void TypeBuilder::visit(type::Iterator* i)
{
}

void TypeBuilder::visit(type::List* l)
{
    auto item = hiltiType(l->elementType());
    auto t = hilti::builder::list::type(item);
    auto bt = t;

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(bt);
    ti.hilti_default = hilti::builder::list::create(item, {}, l->location());
    ti.always_initialize = true;
    setResult(ti);
}

void TypeBuilder::visit(type::Map* m)
{
    auto key = hiltiType(m->keyType());
    auto val = hiltiType(m->valueType());
    auto t = hilti::builder::map::type(key, val);
    auto bt = t;

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(bt);
    ti.hilti_default = hilti::builder::map::create(key, val, {}, nullptr, hilti::AttributeSet(), m->location());
    ti.always_initialize = true;
    setResult(ti);
}

void TypeBuilder::visit(type::Module* m)
{
}

void TypeBuilder::visit(type::Network* n)
{
}

void TypeBuilder::visit(type::OptionalArgument* o)
{
}

void TypeBuilder::visit(type::Optional* o)
{
    shared_ptr<::hilti::Type> ht;

    if ( o->argType() ) {
        ::hilti::builder::type_list t = { hiltiType(o->argType()) };
        ht = ::hilti::builder::union_::type(t, o->location() );
    }

    else
        ht = ::hilti::builder::union_::typeAny(o->location());

    TypeInfo ti;
    ti.hilti_type = _buildType(ht, BINPAC_TYPE_OPTIONAL);
    setResult(ti);
}

void TypeBuilder::visit(type::Port* p)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::port::type();
    setResult(ti);
}

void TypeBuilder::visit(type::RegExp* r)
{
    auto t = hilti::builder::regexp::type();
    auto bt = t;

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(bt);

    setResult(ti);
}

void TypeBuilder::visit(type::Set* s)
{
    auto item = hiltiType(s->elementType());
    auto t = hilti::builder::set::type(item);
    auto bt = t;

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(bt);
    ti.hilti_default = hilti::builder::set::create(item, {}, s->location());
    ti.always_initialize = true;
    setResult(ti);
}

void TypeBuilder::visit(type::String* s)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::string::type();
    setResult(ti);
}

void TypeBuilder::visit(type::Sink* s)
{
    auto t = hilti::builder::type::byName("BinPACHilti::Sink");

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(t);
    setResult(ti);
}

void TypeBuilder::visit(type::Time* t)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::time::type();
    setResult(ti);
}

void TypeBuilder::visit(type::Tuple* t)
{
    hilti::builder::type_list types;

    for ( auto t : t->typeList() )
        types.push_back(hiltiType(t));

    auto tt = hilti::builder::tuple::type(types, t->location());

    if ( t->wildcard() )
        tt->setWildcard(true);

    TypeInfo ti;
    ti.hilti_type = tt;
    setResult(ti);
}

void TypeBuilder::visit(type::TypeByName* t)
{
}

void TypeBuilder::visit(type::TypeType* t)
{
}

void TypeBuilder::visit(type::Unit* u)
{
    assert(u->id());

    shared_ptr<hilti::ID> uid = nullptr;

    if ( cg() ) {
        uid = cg()->hiltiID(u->id(), true);

        if ( uid->isScoped() ) {
            auto id = std::make_shared<ID>(uid->pathAsString());
            cg()->hiltiImportType(id, u->sharedPtr<Type>());

            if ( _deps )
                _deps->push_back(uid);
        }
    }

    else {
        uid = hilti::builder::id::node(u->id()->pathAsString());

        if ( ! uid->isScoped() ) {
            auto mod = u->id()->firstParent<Module>();
            assert(mod);
            string name = util::fmt("%s::%s", mod->id()->name(), uid->name());
            uid = hilti::builder::id::node(name);

            if ( _deps )
                _deps->push_back(uid);
        }

        else if ( u->anonymous() && _deps )
            _deps->push_back(uid);
    }

    auto t = hilti::builder::reference::type(hilti::builder::type::byName(uid));

    TypeInfo ti;
    ti.hilti_type = t;
    ti.hilti_id = uid;

    setResult(ti);
}

void TypeBuilder::visit(type::Unknown* u)
{
}

void TypeBuilder::visit(type::Unset* u)
{
}

void TypeBuilder::visit(type::Vector* v)
{
    auto item = hiltiType(v->elementType());
    auto t = hilti::builder::vector::type(item);
    auto bt = t;

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(bt);
    ti.hilti_default = hilti::builder::vector::create(item, {}, v->location());
    ti.always_initialize = true;
    setResult(ti);
}

void TypeBuilder::visit(type::Void* v)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::void_::type();
    setResult(ti);
}

void TypeBuilder::visit(type::function::Parameter* p)
{
}

void TypeBuilder::visit(type::function::Result* r)
{
}

void TypeBuilder::visit(type::iterator::Bytes* b)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::iterator::typeBytes();
    setResult(ti);
}

void TypeBuilder::visit(type::iterator::List* l)
{
}

void TypeBuilder::visit(type::iterator::Regexp* r)
{
}

void TypeBuilder::visit(type::iterator::Set* s)
{
}

void TypeBuilder::visit(type::iterator::Vector* v)
{
}

void TypeBuilder::visit(type::unit::Item* i)
{
}

void TypeBuilder::visit(type::unit::item::Field* f)
{
}

void TypeBuilder::visit(type::unit::item::GlobalHook* g)
{
}

void TypeBuilder::visit(type::unit::item::Property* p)
{
}

void TypeBuilder::visit(type::unit::item::Variable* v)
{
}

void TypeBuilder::visit(type::unit::item::field::Constant* c)
{
}

void TypeBuilder::visit(type::unit::item::field::Ctor* c)
{
}

void TypeBuilder::visit(type::unit::item::field::Switch* s)
{
}

void TypeBuilder::visit(type::unit::item::field::AtomicType* t)
{
}

void TypeBuilder::visit(type::unit::item::field::Unit* t)
{
}

void TypeBuilder::visit(type::unit::item::field::switch_::Case* c)
{
}
