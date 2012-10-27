
#include "type-builder.h"
#include "type.h"

using namespace binpac;
using namespace binpac::codegen;

TypeBuilder::TypeBuilder(CodeGen* cg)
    : CGVisitor<TypeInfo>(cg, "TypeBuilder")
{
}

TypeBuilder::~TypeBuilder()
{
}

shared_ptr<hilti::Type> TypeBuilder::hiltiType(shared_ptr<Type> type)
{
    TypeInfo result;
    bool success = processOne(type, &result);
    assert(success);

    return result.hilti_type;
}

shared_ptr<hilti::Expression> TypeBuilder::hiltiDefault(shared_ptr<Type> type, bool null_on_default)
{
    TypeInfo result;
    bool success = processOne(type, &result);
    assert(success);

    return result.hilti_default ? result.hilti_default
                                : (null_on_default ? shared_ptr<hilti::Expression>() : hilti::builder::expression::default_(result.hilti_type));
}

void TypeBuilder::visit(type::Address* a)
{
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

    setResult(ti);
}

void TypeBuilder::visit(type::CAddr* c)
{
}

void TypeBuilder::visit(type::Double* d)
{
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

    ti.hilti_default = hilti::builder::id::create(util::fmt("%s::Undef", e->id()->name()));
    ti.hilti_type = hilti::builder::enum_::type(labels, e->location());
    ti.hilti_type->setID(cg()->hiltiID(e->id()));

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
    setResult(ti);
}

void TypeBuilder::visit(type::Map* m)
{
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

void TypeBuilder::visit(type::Port* p)
{
}

void TypeBuilder::visit(type::RegExp* r)
{
    TypeInfo ti;

    auto t = hilti::builder::regexp::type(hilti::builder::regexp::attribute_list(), r->location());
    ti.hilti_type = hilti::builder::reference::type(t, r->location());

    setResult(ti);
}

void TypeBuilder::visit(type::Set* s)
{
}

void TypeBuilder::visit(type::String* s)
{
    TypeInfo ti;
    ti.hilti_type = hilti::builder::string::type(s->location());
    setResult(ti);
}

void TypeBuilder::visit(type::Time* t)
{
}

void TypeBuilder::visit(type::Tuple* t)
{
    hilti::builder::type_list types;

    for ( auto t : t->typeList() )
        types.push_back(cg()->hiltiType(t));

    TypeInfo ti;
    ti.hilti_type = hilti::builder::tuple::type(types, t->location());
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

    TypeInfo ti;
    ti.hilti_type = hilti::builder::reference::type(hilti::builder::type::byName(cg()->hiltiID(u->id())));
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
