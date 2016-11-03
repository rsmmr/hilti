
#include "ctor.h"
#include "expression.h"
#include "id.h"
#include "passes/printer.h"
#include "type.h"

using namespace spicy;
using namespace spicy::ctor;

Ctor::Ctor(const Location& l) : ast::Ctor<AstInfo>(l)
{
}

string Ctor::render()
{
    std::ostringstream s;
    passes::Printer(s, true).run(sharedPtr<Node>());
    return s.str();
}

Ctor::pattern_list Ctor::patterns() const
{
    return {};
}

Bytes::Bytes(const string& b, const Location& l) : Ctor(l)
{
    _value = b;
}

const string& Bytes::value() const
{
    return _value;
}

int Bytes::length() const
{
    return _value.size();
}

shared_ptr<Type> Bytes::type() const
{
    return std::make_shared<type::Bytes>(location());
}

List::List(shared_ptr<Type> etype, const expression_list& elems, const Location& l) : Ctor(l)
{
    assert(etype || elems.size());

    if ( ! etype )
        etype = elems.front()->type();

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
    assert(etype || elems.size());

    if ( ! etype )
        etype = elems.front()->type();

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

Map::Map(shared_ptr<Type> ktype, shared_ptr<Type> vtype, const element_list& elems,
         const Location& l)
    : Ctor(l)
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

RegExp::RegExp(const string& regexp, const attribute_list& attrs, const Location& l) : Ctor(l)
{
    _patterns = {regexp};
    _attributes = attrs;
    _type = std::make_shared<type::RegExp>(l);
}

RegExp::RegExp(const pattern_list& patterns, const attribute_list& attrs, const Location& l)
{
    _patterns = patterns;
    _attributes = attrs;
    _type = std::make_shared<type::RegExp>(l);
    addChild(_type);
}

shared_ptr<Type> RegExp::type() const
{
    return _type;
}

Ctor::pattern_list RegExp::patterns() const
{
    return _patterns;
}

const attribute_list& RegExp::attributes() const
{
    return _attributes;
}

Unit::Unit(const item_list& items, const Location& l) : Ctor(l)
{
    for ( auto i : items ) {
        node_ptr<ID> id = i.first;
        node_ptr<Expression> init = i.second;
        _items.push_back(std::make_pair(id, init));
        addChild(id);
        addChild(init);
    }

    unit_item_list uitems;

    int cnt = 0;

    for ( auto i : _items ) {
        auto id = i.first;
        auto type = i.second->type();
        auto def = nullptr;

        bool no_id = (id == nullptr);

        if ( no_id )
            id = std::make_shared<ID>(::util::fmt("_i%d", ++cnt));

        auto ui = std::make_shared<type::unit::item::Variable>(id, type, def, hook_list(),
                                                               attribute_list(), location());

        if ( no_id )
            ui->setCtorNoName();

        uitems.push_back(ui);
    }

    auto utype = std::make_shared<type::Unit>(parameter_list(), uitems);
    utype->setAnonymous();

    _type = utype;
    addChild(_type);
}

ctor::Unit::item_list Unit::items() const
{
    item_list items;

    for ( auto i : _items )
        items.push_back(i);

    return items;
}

shared_ptr<Type> Unit::type() const
{
    auto utype = ast::rtti::checkedCast<type::Unit>(_type);

    // See if we have resolved some expression types in the meantime.
    for ( auto i : util::zip2(_items, utype->variables()) ) {
        auto type = i.first.second->type();
        auto var = i.second;
        var->setType(type);
    }

    return _type;
}
