
#include <initializer_list>

#include "expression.h"
#include "type.h"
#include "variable.h"
#include "function.h"

using namespace hilti;

type::trait::Trait::~Trait()
{
}

string type::trait::parameter::Type::repr() const
{
    return _type->repr();
}

type::Any::~Any() {}
type::Unknown::~Unknown() {}
type::TypeByName::~TypeByName() {}
type::Unset::~Unset() {}
type::Type::~Type() {}
type::Block::~Block() {}
type::Module::~Module() {}
type::Void::~Void() {}
type::String::~String() {}
type::Integer::~Integer() {}
type::Bool::~Bool() {}
type::Label::~Label() {}
type::Bytes::~Bytes() {};

string type::trait::Parameterized::repr() const
{
    if ( dynamic_cast<const hilti::Type*>(this)->wildcard() )
        return reprPrefix() + "<*>";

    string t = reprPrefix() + "<";

    bool first = true;

    for ( auto p : parameters() ) {
        if ( ! first )
            t += ",";

        t += p->repr();
        first = false;
    }

    return t + ">";
}

type::Function::Function(shared_ptr<hilti::function::Parameter> result, const function::parameter_list& args, hilti::type::function::CallingConvention cc, const Location& l)
    : hilti::Type(l), ast::type::mixin::Function<AstInfo>(this, result, args)
{
    _cc = cc;
}

type::Function::Function(const Location& l)
    : hilti::Type(l),
    ast::type::mixin::Function<AstInfo>(this, shared_ptr<hilti::function::Parameter>(new hilti::function::Parameter(shared_ptr<hilti::Type>(new type::Void()), false)), function::parameter_list())
{
    setWildcard(true);
    _cc = function::CallingConvention::DEFAULT; // Doesn't matter.
}

type::function::Parameter::Parameter(shared_ptr<hilti::ID> id, shared_ptr<Type> type, bool constant, shared_ptr<Expression> default_value, Location l)
    : ast::type::mixin::function::Parameter<AstInfo>(id, type, constant, default_value, l)
{
}

type::function::Parameter::Parameter(shared_ptr<Type> type, bool constant, Location l)
    : ast::type::mixin::function::Parameter<AstInfo>(type, constant, l)
{
}

type::Tuple::Tuple(const Location& l) : ValueType(l)
{
    setWildcard(true);
}

type::Tuple::Tuple(const type_list& types, const Location& l) : ValueType(l)
{
    _types = types;

    for ( auto t : types ) {
        auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(t));
        _parameters.push_back(p);
        addChild(p);
        addChild(t);
    }
}

const type::Tuple::type_list& type::Tuple::typeList() const
{
    return _types;
}

type::Reference::Reference(const Location& l) : ValueType(l)
{
    setWildcard(true);
}

type::Reference::Reference(shared_ptr<Type> rtype, const Location& l) : ValueType(l)
{
    _rtype = rtype;
    addChild(_rtype);

    auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(rtype));
    _parameters.push_back(p);
    addChild(p);
}

type::Iterator::Iterator(const Location& l) : ValueType(l)
{
    setWildcard(true);
}

type::Iterator::Iterator(shared_ptr<Type> ttype, const Location& l) : ValueType(l)
{
    _ttype = ttype;
    addChild(_ttype);

    auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(ttype));
    _parameters.push_back(p);
    addChild(p);
}

shared_ptr<Type> type::Bytes::iterType() const
{
    return shared_ptr<Type>(new type::iterator::Bytes(location()));
}

// FIXME: We won't need this anymore with C++11 initializer lists.
template< typename T, size_t N >
std::vector<T> makeVector( const T (&data)[N] )
{
    return std::vector<T>(data, data+N);

}

template<typename T>
shared_ptr<T> sharedPtr(T* t)
{
    return shared_ptr<T>(t);
}

type::trait::Unpackable::Format _unpack_formats_bytes[] = {
    { "BytesRunLength", sharedPtr(new type::TypeByName("Hilti::Packed")), false,
      "A series of bytes preceded by an uint indicating its length." },

    { "BytesFixed", sharedPtr(new type::Integer(64)), false,
      "A series of bytes of fixed length specified by an additional integer argument" },

    { "BytesDelim", sharedPtr(new type::Reference(type::Bytes())), false,
      "A series of bytes delimited by a final byte-sequence specified by an additional argument." },

    { "SkipBytesRunLength", sharedPtr(new type::TypeByName("Hilti::Packed")), false,
      "Like BytesRunLength, but does not return unpacked value." },

    { "SkipBytesFixed", sharedPtr(new type::Integer(64)), false,
      "Like BytesFixed, but does not return unpacked value." },

    { "SkipBytesDelim", sharedPtr(new type::Reference(type::Bytes())), false,
      "Like BytesDelim, but does not return unpacked value." },
};

std::vector<type::trait::Unpackable::Format> unpack_formats_bytes(makeVector(_unpack_formats_bytes));

const std::vector<type::trait::Unpackable::Format>& type::Bytes::unpackFormats() const
{
    return unpack_formats_bytes;
}

