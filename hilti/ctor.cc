
#include "id.h"
#include "ctor.h"
#include "hilti-intern.h"
#include "expression.h"

using namespace hilti;

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

ctor::RegExp::RegExp(const pattern_list& patterns, const Location& l)
{
    _patterns = patterns;
    _type = std::make_shared<type::RegExp>(type::RegExp::attribute_list(), l);
    _type = std::make_shared<type::Reference>(_type);
    addChild(_type);
}
