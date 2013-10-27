
#include "id.h"
#include "ctor.h"
#include "hilti-intern.h"
#include "expression.h"

using namespace hilti;

std::list<shared_ptr<hilti::Expression>> Ctor::flatten()
{
    return {};
}

shared_ptr<Type> ctor::Bytes::type() const
{
    auto b = shared_ptr<type::Bytes>(new type::Bytes(location()));
    return shared_ptr<type::Reference>(new type::Reference(b, location()));
}

ctor::List::List(shared_ptr<Type> etype, const element_list& elems, const Location& l) : Ctor(l)
{
    assert(etype);

    _elems = elems;
    _type = std::make_shared<type::List>(etype);
    _type = std::make_shared<type::Reference>(_type);

    for ( auto e : _elems )
        addChild(e);

    addChild(_type);
}

std::list<shared_ptr<hilti::Expression>> ctor::List::flatten()
{
    std::list<shared_ptr<hilti::Expression>> l;

    for ( auto e : _elems )
        l.merge(e->flatten());

    return l;
}

ctor::Vector::Vector(shared_ptr<Type> etype, const element_list& elems, const Location& l) : Ctor(l)
{
    assert(etype);

    _elems = elems;
    _type = std::make_shared<type::Vector>(etype);
    _type = std::make_shared<type::Reference>(_type);

    for ( auto e : _elems )
        addChild(e);

    addChild(_type);
}

std::list<shared_ptr<hilti::Expression>> ctor::Vector::flatten()
{
    std::list<shared_ptr<hilti::Expression>> l;

    for ( auto e : _elems )
        l.merge(e->flatten());

    return l;
}

ctor::Set::Set(shared_ptr<Type> etype, const element_list& elems, const Location& l) : Ctor(l)
{
    assert(etype);

    _elems = elems;
    _type = std::make_shared<type::Set>(etype);
    _type = std::make_shared<type::Reference>(_type);

    for ( auto e : _elems )
        addChild(e);

    addChild(_type);
}

ctor::Set::Set(bool cctor, shared_ptr<Type> stype, const element_list& elems, const Location& l)
{
    assert(stype);

    _elems = elems;
    _type = ast::checkedCast<type::Set>(stype);
    _type = std::make_shared<type::Reference>(_type);

    for ( auto e : _elems )
        addChild(e);

    addChild(_type);
}

std::list<shared_ptr<hilti::Expression>> ctor::Set::flatten()
{
    std::list<shared_ptr<hilti::Expression>> l;

    for ( auto e : _elems )
        l.merge(e->flatten());

    return l;
}

ctor::Map::Map(shared_ptr<Type> ktype, shared_ptr<Type> vtype, const element_list& elems, const Location& l) : Ctor(l)
{
    assert(ktype);
    assert(vtype);

    _elems = elems;
    _type = std::make_shared<type::Map>(ktype, vtype, l);
    _type = std::make_shared<type::Reference>(_type);

    for ( auto e : _elems ) {
        addChild(e.first);
        addChild(e.second);
    }

    addChild(_type);
}

ctor::Map::Map(shared_ptr<Type> mtype, const element_list& elems, const Location& l)
{
    assert(mtype);

    _elems = elems;
    _type = ast::checkedCast<type::Map>(mtype);
    _type = std::make_shared<type::Reference>(_type);

    for ( auto e : _elems ) {
        addChild(e.first);
        addChild(e.second);
    }

    addChild(_type);
}

std::list<shared_ptr<hilti::Expression>> ctor::Map::flatten()
{
    std::list<shared_ptr<hilti::Expression>> l;

    for ( auto e : _elems ) {
        l.merge(e.first->flatten());
        l.merge(e.second->flatten());
    }

    return l;

}

ctor::RegExp::RegExp(const pattern_list& patterns, const Location& l)
{
    _patterns = patterns;
    _type = std::make_shared<type::RegExp>(type::RegExp::attribute_list(), l);
    _type = std::make_shared<type::Reference>(_type);
    addChild(_type);
}
