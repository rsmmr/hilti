
#include "id.h"
#include "attribute.h"
#include "hilti-intern.h"

namespace hilti {
namespace attribute {

enum AttributeValueType {
    NONE,
    EXPRESSION,
    INTEGER,
    STRING,
    TYPE,
};

}
}

using namespace hilti;

typedef struct {
    attribute::Tag tag;
    attribute::Context context;
    attribute::AttributeValueType value;
    const char* name;
    const char* doc;
} AttributeDef;

static const AttributeDef Attributes[] = {
    { attribute::CANREMOVE,     attribute::STRUCT_FIELD, attribute::NONE,       "canremove",     "<insert doc>" },
    { attribute::DEFAULT,       attribute::STRUCT_FIELD, attribute::EXPRESSION, "default",       "<insert doc>" },
    { attribute::DEFAULT,       attribute::MAP ,         attribute::EXPRESSION, "default",       "<insert doc>" },
    { attribute::GROUP,         attribute::FUNCTION,     attribute::INTEGER,    "group",         "<insert doc>" },
    { attribute::HOIST,         attribute::VARIABLE,     attribute::NONE,       "hoist",         "<insert doc>" },
    { attribute::LIBHILTI,      attribute::EXCEPTION,    attribute::STRING,     "libhilti",      "<insert doc>" },
    { attribute::LIBHILTI_DTOR, attribute::STRUCT,       attribute::STRING,     "libhilti_dtor", "<insert doc>" },
    { attribute::MAYYIELD,      attribute::FUNCTION,     attribute::NONE,       "mayyield",      "<insert doc>" },
    { attribute::NOEXCEPTION,   attribute::FUNCTION,     attribute::NONE,       "noexception",   "<insert doc>" },
    { attribute::NOSAFEPOINT,   attribute::FUNCTION,     attribute::NONE,       "nosafepoint",   "<insert doc>" },
    { attribute::NOSUB,         attribute::REGEXP,       attribute::NONE,       "nosub",         "<insert doc>" },
    { attribute::NOYIELD,       attribute::FUNCTION,     attribute::NONE,       "noyield",       "<insert doc>" },
    { attribute::PRIORITY,      attribute::FUNCTION,     attribute::INTEGER,    "priority",      "<insert doc>" },
    { attribute::REF,           attribute::FUNCTION,     attribute::NONE,       "ref",           "<insert doc>" },
    { attribute::SAFEPOINT,     attribute::FUNCTION,     attribute::NONE,       "safepoint",     "<insert doc>" },
    { attribute::SCOPE,         attribute::FUNCTION,     attribute::TYPE,       "scope",         "<insert doc>" },
    ///
    { attribute::ERROR,         attribute::ANY,          attribute::NONE,       "<error>",       "<internal error marker; should not be visible>" },
};

Attribute::Attribute(attribute::Tag tag)
{
    _tag = tag;
}

Attribute::Attribute(attribute::Tag tag, node_ptr<Node> value)
{
    _tag = tag;
    _value = value;
}

Attribute::Attribute(attribute::Tag tag, int64_t value)
{
    _tag = tag;
    _value = builder::integer::create(value);
}

Attribute::Attribute(attribute::Tag tag, const std::string& value)
{
    _tag = tag;
    _value = builder::string::create(value);
}

attribute::Tag Attribute::tag() const
{
    return _tag;
}

bool Attribute::hasValue() const
{
    return _value.get();
}

shared_ptr<Node> Attribute::value() const
{
    return _value;
}

bool Attribute::validate(attribute::Context ctx, std::string* msg) const
{
    // TODO: Implement.
    return true;
}

std::string Attribute::render() const
{
    if ( ! _value )
        return string("&") + tagToName(_tag);

    return ::util::fmt("&%s=%s", tagToName(_tag), _value->render());
}

Attribute::operator bool() const
{
    return _tag != attribute::ERROR;
}

string Attribute::tagToName(attribute::Tag tag)
{
    for ( int i = 0; Attributes[i].tag != attribute::ERROR; i++ ) {
        if ( Attributes[i].tag == tag )
            return Attributes[i].name;
    }

    fprintf(stderr, "internal error: attribute tag %d not listed in table\n", (int)tag);
    abort();
}

attribute::Tag Attribute::nameToTag(std::string name)
{
    name = ::util::strreplace(name, "&", "");

    for ( int i = 0; Attributes[i].tag != attribute::ERROR; i++ ) {
        if ( Attributes[i].name == name )
            return Attributes[i].tag;
    }

    return attribute::ERROR;
}

AttributeSet::AttributeSet()
{
    _attributes.resize(attribute::ERROR);
}

bool AttributeSet::has(attribute::Tag tag) const
{
    return _attributes[tag].tag() != attribute::ERROR;
}

Attribute AttributeSet::get(attribute::Tag tag) const
{
    return _attributes[tag];
}

int64_t AttributeSet::getAsInt(attribute::Tag tag, int64_t default_) const
{
    auto a = _attributes[tag];

    if ( ! a._value )
        return default_;

    auto c = ast::checkedCast<expression::Constant>(a._value);
    return ast::checkedCast<constant::Integer>(c->constant())->value();
}

std::string AttributeSet::getAsString(attribute::Tag tag, const std::string& default_) const
{
    auto a = _attributes[tag];

    if ( ! a._value )
        return default_;

    auto c = ast::checkedCast<expression::Constant>(a._value);
    return ast::checkedCast<constant::String>(c->constant())->value();
}

shared_ptr<Type> AttributeSet::getAsType(attribute::Tag tag, shared_ptr<Type> type) const
{
    auto a = _attributes[tag];

    if ( ! a._value )
        return nullptr;

    auto t = ast::checkedCast<expression::Type>(a._value);
    auto tt = ast::checkedCast<type::TypeType>(t->type());
    return tt->typeType();
}

shared_ptr<Expression> AttributeSet::getAsExpression(attribute::Tag tag, shared_ptr<Expression> default_) const
{
    auto a = _attributes[tag];

    if ( ! a._value )
        return nullptr;

    return ast::checkedCast<Expression>(a._value);
}

void AttributeSet::add(Attribute attr)
{
    auto tag = attr.tag();

    _attributes[tag] = attr;

    if ( attr.value() )
        addChild(_attributes[tag]._value);
}

void AttributeSet::add(attribute::Tag tag, shared_ptr<Node> value)
{
    add(Attribute(tag, value));
}

void AttributeSet::add(attribute::Tag tag, int64_t value)
{
    add(Attribute(tag, value));
}

void AttributeSet::add(attribute::Tag tag, const std::string& value)
{
    add(Attribute(tag, value));
}

void AttributeSet::add(attribute::Tag attr)
{
    add(Attribute(attr));
}

void AttributeSet::remove(Attribute attr)
{
    remove(attr.tag());
}

void AttributeSet::remove(attribute::Tag tag)
{
    if ( _attributes[tag]._value )
        removeChild(_attributes[tag]._value);

    _attributes[tag] = Attribute();
}

bool AttributeSet::validate(attribute::Context ctx, std::string* msg) const
{
    for ( int i = 0; i < attribute::ERROR; i++ ) {
        if ( _attributes[i] && ! _attributes[i].validate(ctx, msg) )
            return false;
    }

    return true;
}

std::list<Attribute> AttributeSet::all() const
{
    std::list<Attribute> all;

    for ( int i = 0; i < attribute::ERROR; i++ ) {
        if ( _attributes[i] )
            all.push_back(_attributes[i]);
    }

    return all;
}

std::string AttributeSet::render()
{
    std::string s;

    for ( int i = 0; i < attribute::ERROR; i++ ) {
        if ( ! _attributes[i] )
            continue;

        if ( s.size() )
            s += " ";

        s += _attributes[i].render();
    }

    return s;
}

AttributeSet& AttributeSet::operator=(const AttributeSet& other)
{
    for ( int i = 0; i < attribute::ERROR; i++ ) {
        if ( _attributes[i] )
            remove(_attributes[i]);
    }

    _attributes = other._attributes;

    for ( int i = 0; i < attribute::ERROR; i++ ) {
        if ( _attributes[i] )
            add(_attributes[i]);
    }

    return *this;
}
