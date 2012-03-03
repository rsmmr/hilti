
#include "id.h"
#include "ctor.h"
#include "hilti.h"
#include "expression.h"

using namespace hilti;

shared_ptr<Type> ctor::Bytes::type() const
{
    auto b = shared_ptr<type::Bytes>(new type::Bytes(location()));
    return shared_ptr<type::Reference>(new type::Reference(b, location()));
}

ctor::List::List(const element_list& elems, const Location& l) : Ctor(l)
{
    _elems = elems;

    for ( auto e : _elems )
        addChild(e);

    if ( elems.size() )
        _type = std::make_shared<type::List>(elems.front()->type(), l);
    else
        // Wildcard type.
        _type = std::make_shared<type::List>(l);

    _type = std::make_shared<type::Reference>(_type);

    addChild(_type);
}

ctor::Vector::Vector(const element_list& elems, const Location& l) : Ctor(l)
{
    _elems = elems;

    for ( auto e : _elems )
        addChild(e);

    if ( elems.size() )
        _type = std::make_shared<type::Vector>(elems.front()->type(), l);
    else
        // Wildcard type.
        _type = std::make_shared<type::Vector>(l);

    _type = std::make_shared<type::Reference>(_type);

    addChild(_type);
}

ctor::Set::Set(const element_list& elems, const Location& l) : Ctor(l)
{
    _elems = elems;

    for ( auto e : _elems )
        addChild(e);

    if ( elems.size() )
        _type = std::make_shared<type::Set>(elems.front()->type(), l);
    else
        // Wildcard type.
        _type = std::make_shared<type::Set>(l);

    _type = std::make_shared<type::Reference>(_type);

    addChild(_type);
}

ctor::Map::Map(const element_list& elems, const Location& l) : Ctor(l)
{
    _elems = elems;

    for ( auto e : _elems ) {
        addChild(e.first);
        addChild(e.second);
    }

    if ( elems.size() ) {
        auto front = elems.front();
        _type = std::make_shared<type::Map>(front.first->type(), front.second->type(), l);
    }
    else
        // Wildcard type.
        _type = std::make_shared<type::Map>(l);

    _type = std::make_shared<type::Reference>(_type);

    addChild(_type);
}
