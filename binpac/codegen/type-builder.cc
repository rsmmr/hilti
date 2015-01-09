
#include "type-builder.h"
#include "../type.h"
#include "../module.h"

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

shared_ptr<hilti::Type> TypeBuilder::hiltiType(shared_ptr<Type> type, id_list* deps)
{
    _deps = deps;

    TypeInfo result;
    bool success = processOne(type, &result);
    assert(success);

    _deps = nullptr;

    return result.hilti_type;
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

void TypeBuilder::visit(type::Address* a)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::address::type(a->location());
    setResult(ti);
}

void TypeBuilder::visit(type::Any* a)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::any::type(a->location());
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
    ti.hilti_type = hilti::builder::boolean::type( b->location());

    setResult(ti);
}

void TypeBuilder::visit(type::Bytes* b)
{
    TypeInfo ti;

    auto t = hilti::builder::bytes::type(b->location());
    ti.hilti_type = hilti::builder::reference::type(t, b->location());
    ti.hilti_default = hilti::builder::bytes::create("", b->location());

    setResult(ti);
}

void TypeBuilder::visit(type::CAddr* c)
{
}

void TypeBuilder::visit(type::Double* d)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::double_::type(d->location());
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

    ti.hilti_type = hilti::builder::enum_::type(labels, e->location());
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
    ti.hilti_type = etype;
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
    TypeInfo ti;
    ti.hilti_type = hilti::builder::integer::type(i->width());
    setResult(ti);
}

void TypeBuilder::visit(type::Interval* i)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::interval::type(i->location());
    setResult(ti);
}

void TypeBuilder::visit(type::Iterator* i)
{
}

void TypeBuilder::visit(type::List* l)
{
    auto item = hiltiType(l->elementType());

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(hilti::builder::list::type(item, l->location()));
    ti.hilti_default = hilti::builder::list::create(item, {}, l->location());
    ti.always_initialize = true;
    setResult(ti);
}

void TypeBuilder::visit(type::Map* m)
{
    auto key = hiltiType(m->keyType());
    auto val = hiltiType(m->valueType());

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(hilti::builder::map::type(key, val, m->location()));
    ti.hilti_default = hilti::builder::map::create(key, val, {}, nullptr, m->location());
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
    ti.hilti_type = ht;
    setResult(ti);
}

void TypeBuilder::visit(type::Port* p)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::port::type(p->location());
    setResult(ti);
}

void TypeBuilder::visit(type::RegExp* r)
{
    TypeInfo ti;

    auto t = hilti::builder::regexp::type(r->location());
    ti.hilti_type = hilti::builder::reference::type(t, r->location());

    setResult(ti);
}

void TypeBuilder::visit(type::Set* s)
{
    auto item = hiltiType(s->elementType());

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(hilti::builder::set::type(item, s->location()));
    ti.hilti_default = hilti::builder::set::create(item, {}, s->location());
    ti.always_initialize = true;
    setResult(ti);
}

void TypeBuilder::visit(type::String* s)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::string::type(s->location());
    setResult(ti);
}

void TypeBuilder::visit(type::Sink* s)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::Sink"));
    setResult(ti);
}

void TypeBuilder::visit(type::Time* t)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::time::type(t->location());
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

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(hilti::builder::type::byName(uid));
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

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(hilti::builder::vector::type(item, v->location()));
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
    ti.hilti_type = hilti::builder::iterator::typeBytes(b->location());
    setResult(ti);
}

void TypeBuilder::visit(type::iterator::List* l)
{
    auto ltype = ast::checkedCast<type::List>(l->argType());
    auto etype = hiltiType(ltype->elementType());
    auto ctype = hilti::builder::list::type(etype);

    TypeInfo ti;
    ti.hilti_type = std::make_shared<hilti::type::iterator::List>(ctype, l->location());
    setResult(ti);
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
