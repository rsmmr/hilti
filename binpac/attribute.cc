
#include "attribute.h"
#include "type.h"
#include "expression.h"
#include "function.h"
#include "id.h"

using namespace binpac;

Attribute::Attribute(const string& key, shared_ptr<Expression> value, const Location& l) : Node(l)
{
    string k = key;

    while ( k.size() && k[0] == '&' )
        k = k.substr(1, std::string::npos);

    _key = k;
    _value = value;

    addChild(_value);
}

const string& Attribute::key() const
{
    return _key;
}


shared_ptr<Expression> Attribute::value() const
{
    return _value;
}

AttributeSet::AttributeSet(const attribute_list& attrs, const Location& l) : Node(l)
{
    for ( auto a : attrs )
        add(a);
}

shared_ptr<Attribute> AttributeSet::add(shared_ptr<Attribute> attr)
{
    shared_ptr<Attribute> old = nullptr;

    auto i = _attrs.find(attr->key());

    if ( i != _attrs.end() ) {
        old = i->second;
        _attrs.erase(i);
        removeChild(old);
    }

    auto p = _attrs.insert(std::make_pair(attr->key(), attr));
    addChild(p.first->second);

    return old;
}

shared_ptr<Attribute> AttributeSet::remove(const string& key)
{
    shared_ptr<Attribute> old = nullptr;

    auto i = _attrs.find(key);

    if ( i != _attrs.end() ) {
        old = i->second;
        _attrs.erase(i);
        removeChild(old);
    }

    return old;
}

bool AttributeSet::has(const string& key) const
{
    return lookup(key) != nullptr;
}

shared_ptr<Attribute> AttributeSet::lookup(const string& key) const
{
    auto i = _attrs.find(key);
    return i != _attrs.end() ? i->second : nullptr;
}

std::list<shared_ptr<Attribute>> AttributeSet::attributes() const
{
    std::list<shared_ptr<Attribute>> attrs;

    for ( auto i : _attrs )
        attrs.push_back(i.second);

    return attrs;
}


AttributeSet& AttributeSet::operator=(const attribute_list& attrs)
{
    _attrs.clear();

    for ( auto a : attrs )
        add(a);

    return *this;
}
