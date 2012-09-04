
#include "ctor.h"
#include "id.h"
#include "expression.h"
#include "type.h"
#include "passes/printer.h"

using namespace binpac;
using namespace binpac::ctor;

Ctor::Ctor(const Location& l) : ast::Ctor<AstInfo>(l)
{
}

string Ctor::render()
{
    std::ostringstream s;
    passes::Printer(s, true).run(sharedPtr<Node>());
    return s.str();
}

Bytes::Bytes(const string& b, const Location& l) : Ctor(l)
{
    _value = b;
}

const string& Bytes::value() const
{
    return _value;
}

shared_ptr<Type> Bytes::type() const
{
    return std::make_shared<type::Bytes>(location());
}

List::List(shared_ptr<Type> etype, const expression_list& elems, const Location& l) : Ctor(l)
{
    assert(etype);

    for ( auto e : elems )
        _elems.push_back(e);

    for ( auto e : _elems )
        addChild(e);

    _type = std::make_shared<type::List>(etype);
    addChild(_type);
}

expression_list List::elements() const
{
    expression_list elems;

    for ( auto e : _elems )
        elems.push_back(e);

    return elems;
}

shared_ptr<Type> List::type() const
{
    return _type;
}

Vector::Vector(shared_ptr<Type> etype, const expression_list& elems, const Location& l) : Ctor(l)
{
    assert(etype);

    for ( auto e : elems )
        _elems.push_back(e);

    for ( auto e : _elems )
        addChild(e);

    _type = std::make_shared<type::Vector>(etype);
    addChild(_type);
}

shared_ptr<Type> Vector::type() const
{
    return _type;
}

expression_list Vector::elements() const
{
    expression_list elems;

    for ( auto e : _elems )
        elems.push_back(e);

    return elems;
}

Set::Set(shared_ptr<Type> etype, const expression_list& elems, const Location& l) : Ctor(l)
{
    assert(etype);

    for ( auto e : elems )
        _elems.push_back(e);

    for ( auto e : _elems )
        addChild(e);

    _type = std::make_shared<type::Set>(etype);
    addChild(_type);
}

shared_ptr<Type> Set::type() const
{
    return _type;
}

expression_list Set::elements() const
{
    expression_list elems;

    for ( auto e : _elems )
        elems.push_back(e);

    return elems;
}

Map::Map(shared_ptr<Type> ktype, shared_ptr<Type> vtype, const element_list& elems, const Location& l) : Ctor(l)
{
    assert(ktype);
    assert(vtype);

    for ( auto e : elems )
        _elems.push_back(e);

    for ( auto e : _elems ) {
        addChild(e.first);
        addChild(e.second);
    }

    _type = std::make_shared<type::Map>(ktype, vtype, l);
    addChild(_type);
}

shared_ptr<Type> Map::type() const
{
    return _type;
}

Map::element_list Map::elements() const
{
    element_list elems;

    for ( auto e : _elems ) {
        shared_ptr<Expression> e1 = e.first;
        shared_ptr<Expression> e2 = e.second;
        elems.push_back(std::make_pair(e1, e2));
    }

    return elems;
}

RegExp::RegExp(const string& regexp, const string& flags, const attribute_list& attrs, const Location& l)
{
    _patterns = { pattern(regexp, flags) };
    _type = std::make_shared<type::RegExp>(attrs, l);
}

RegExp::RegExp(const pattern_list& patterns, const attribute_list& attrs, const Location& l)
{
    _patterns = patterns;
    _type = std::make_shared<type::RegExp>(attrs, l);
    addChild(_type);
}

shared_ptr<Type> RegExp::type() const
{
    return _type;
}

const RegExp::pattern_list& RegExp::patterns() const
{
    return _patterns;
}
