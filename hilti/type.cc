
#include <initializer_list>

#include "expression.h"
#include "type.h"
#include "variable.h"
#include "function.h"
#include "passes/printer.h"

using namespace hilti;

string Type::render()
{
    std::ostringstream s;
    passes::Printer(s, true).run(sharedPtr<Node>());
    return s.str();
}

type::trait::Trait::~Trait()
{
}

type::Address::~Address() {}
type::Any::~Any() {}
type::Bitset::~Bitset() {}
type::Block::~Block() {}
type::Bool::~Bool() {}
type::Bytes::~Bytes() {}
type::CAddr::~CAddr() {}
type::Callable::~Callable() {}
type::Channel::~Channel() {}
type::Classifier::~Classifier() {}
type::Double::~Double() {}
type::Enum::~Enum() {}
type::Exception::~Exception() {}
type::File::~File() {}
type::Hook::~Hook() {}
type::IOSource::~IOSource() {}
type::Integer::~Integer() {}
type::Interval::~Interval() {}
type::Label::~Label() {}
type::List::~List() {}
type::Map::~Map() {}
type::MatchTokenState::~MatchTokenState() {}
type::Module::~Module() {}
type::Network::~Network() {}
type::Overlay::~Overlay() {}
type::Port::~Port() {}
type::RegExp::~RegExp() {}
type::Set::~Set() {}
type::String::~String() {}
type::Struct::~Struct() {}
type::Time::~Time() {}
type::Timer::~Timer() {}
type::TimerMgr::~TimerMgr() {}
type::Type::~Type() {}
type::TypeByName::~TypeByName() {}
type::Unknown::~Unknown() {}
type::Unset::~Unset() {}
type::Vector::~Vector() {}
type::Void::~Void() {}


bool type::trait::Parameterized::equal(shared_ptr<hilti::Type> other) const
{
    auto pother = std::dynamic_pointer_cast<type::trait::Parameterized>(other);
    assert(pother);

    auto params = parameters();
    auto oparams = pother->parameters();

    if ( params.size() != oparams.size() )
        return false;

    auto i1 = params.begin();
    auto i2 = oparams.begin();

    for ( ; i1 != params.end(); ++i1, ++i2 ) {
        if ( ! (*i1 && *i2) )
            return false;

        if ( typeid(**i1) != typeid(**i2) )
            return false;

        if ( ! (*i1)->_equal(*i2) )
            return false;
    }

    return true;
}

type::TypedHeapType::TypedHeapType(const Location& l) : HeapType(l)
{
    setWildcard(true);
}

type::TypedHeapType::TypedHeapType(shared_ptr<Type> rtype, const Location& l) : HeapType(l)
{
    _argtype = rtype;
    addChild(_argtype);
}

type::trait::Parameterized::parameter_list type::TypedHeapType::parameters() const
{
    parameter_list params;

    if ( ! _argtype )
        return params;

    auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(_argtype));
    params.push_back(p);
    return params;
}

type::TypedValueType::TypedValueType(const Location& l) : ValueType(l)
{
    setWildcard(true);
}

type::TypedValueType::TypedValueType(shared_ptr<Type> rtype, const Location& l) : ValueType(l)
{
    _argtype = rtype;
    addChild(_argtype);
}

type::trait::Parameterized::parameter_list type::TypedValueType::parameters() const
{
    parameter_list params;

    if ( ! _argtype )
        return params;

    auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(_argtype));
    params.push_back(p);
    return params;
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

    for ( auto t : _types )
        addChild(t);
}

type::trait::Parameterized::parameter_list type::Tuple::parameters() const
{
    parameter_list params;

    for ( auto t : _types ) {
        auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(t));
        params.push_back(p);
    }

    return params;
}

const type::Tuple::type_list type::Tuple::typeList() const
{
    return _types;
}

type::trait::Parameterized::parameter_list type::Integer::parameters() const
{
    parameter_list params;
    auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Integer(_width));
    params.push_back(p);
    return params;
}

type::trait::Parameterized::parameter_list type::IOSource::parameters() const
{
    parameter_list params;
    auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Enum(_kind));
    params.push_back(p);
    return params;
}

type::trait::Parameterized::parameter_list type::Map::parameters() const
{
    auto key = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(_key));
    auto value = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(_value));

    parameter_list params;
    params.push_back(key);
    params.push_back(value);

    return params;
}

type::Exception::Exception(shared_ptr<Type> base, shared_ptr<Type> arg, const Location& l) : TypedHeapType(arg, l)
{
    if ( base ) {
        _base = ast::as<type::Exception>(base);
        assert(_base);
    }
    else
        _base = nullptr;

    addChild(_base);
}

shared_ptr<Type> type::Bytes::iterType()
{
    return shared_ptr<Type>(new type::iterator::Bytes(location()));
}

shared_ptr<Type> type::IOSource::iterType()
{
    return std::make_shared<iterator::IOSource>(sharedPtr<Type>(), location());
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

type::Bitset::Bitset(const label_list& labels, const Location& l) : ValueType(l)
{
    int next = 0;
    for ( auto label : labels ) {
        auto bit = label.second;

        if ( bit < 0 )
            bit = next;

        next = std::max(next, bit + 1);

        _labels.push_back(make_pair(label.first, bit));
    }

    _labels.sort();
}

shared_ptr<Scope> type::Bitset::typeScope()
{
    if ( _scope )
        return _scope;

    _scope = shared_ptr<Scope>(new Scope());

    for ( auto label : _labels ) {
        auto p = shared_from_this();
        auto p2 = std::dynamic_pointer_cast<hilti::Type>(p);
        constant::Bitset::bit_list bl;
        bl.push_back(label.first);
        auto val = shared_ptr<Constant>(new constant::Bitset(bl, p2, location()));
        auto expr = shared_ptr<expression::Constant>(new expression::Constant(val, location()));
        _scope->insert(label.first, expr);
    }

    return _scope;
}

int type::Bitset::labelBit(shared_ptr<ID> label) const
{
    for ( auto l : _labels ) {
        if ( *l.first == *label )
            return l.second;
    }

    throw ast::InternalError(util::fmt("unknown bitset label %s", label->pathAsString().c_str()), this);
}

bool type::Bitset::_equal(shared_ptr<Type> other) const
{
    auto bother = std::dynamic_pointer_cast<type::Bitset>(other);
    assert(bother);

    if ( _labels.size() != bother->_labels.size() )
        return false;

    auto i1 = _labels.begin();
    auto i2 = bother->_labels.begin();

    for ( ; i1 != _labels.end(); ++i1, ++i2 ) {
        if ( i1->first != i2->first || i1->second != i2->second )
            return false;
    }

    return true;
}

type::Enum::Enum(const label_list& labels, const Location& l) : ValueType(l)
{
    int next = 1;
    for ( auto label : labels ) {
        if ( *label.first == "Undef" )
            throw ast::RuntimeError("enum label 'Undef' is already predefined", this);

        auto i = label.second;

        if ( i < 0 )
            i = next;

        next = std::max(next, i + 1);

        _labels.push_back(make_pair(label.first, i));
    }

    _labels.push_back(make_pair(std::make_shared<ID>("Undef"), -1));
    _labels.sort();
}

shared_ptr<Scope> type::Enum::typeScope()
{
    if ( _scope )
        return _scope;

    _scope = shared_ptr<Scope>(new Scope());

    for ( auto label : _labels ) {
        auto p = shared_from_this();
        auto p2 = std::dynamic_pointer_cast<hilti::Type>(p);
        auto val = shared_ptr<Constant>(new constant::Enum(label.first, p2, location()));
        auto expr = shared_ptr<expression::Constant>(new expression::Constant(val, location()));
        _scope->insert(label.first, expr);
    }

    return _scope;
}

int type::Enum::labelValue(shared_ptr<ID> label) const
{
    for ( auto l : _labels ) {
        if ( *l.first == *label )
            return l.second;
    }

    throw ast::InternalError(util::fmt("unknown enum label %s", label->pathAsString().c_str()), this);
    return -1;
}

bool type::Enum::_equal(shared_ptr<Type> other) const
{
    auto eother = std::dynamic_pointer_cast<type::Enum>(other);
    assert(eother);

    if ( _labels.size() != eother->_labels.size() )
        return false;

    auto i1 = _labels.begin();
    auto i2 = eother->_labels.begin();

    for ( ; i1 != _labels.end(); ++i1, ++i2 ) {
        if ( i1->first != i2->first || i1->second != i2->second )
            return false;
    }

    return true;
}

type::trait::Parameterized::parameter_list type::RegExp::parameters() const
{
    parameter_list params;

    for ( auto a : _attrs ) {
        auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Attribute(a));
        params.push_back(p);
    }

    return params;
}

type::struct_::Field::Field(shared_ptr<ID> id, shared_ptr<hilti::Type> type, shared_ptr<Expression> default_, bool internal, const Location& l)
    : Node(l), _id(id), _type(type), _default(default_), _internal(internal)
{
    addChild(id);
    addChild(type);
    addChild(default_);
}

shared_ptr<Expression> type::struct_::Field::default_() const
{
    return _default;
}

type::Struct::Struct(const Location& l) : ValueType(l)
{
    setWildcard(true);
}

type::Struct::Struct(const field_list& fields, const Location& l) : ValueType(l)
{
    _fields = fields;

    for ( auto f : _fields )
        addChild(f);
}

const type::Struct::type_list type::Struct::typeList() const
{
    type_list types;

    for ( auto f : _fields )
        types.push_back(f->type());

    return types;
}

bool type::Struct::_equal(shared_ptr<hilti::Type> ty) const
{
    auto other = ast::as<type::Struct>(ty);

    if ( _fields.size() != other->_fields.size() )
        return false;

    auto i1 = _fields.begin();
    auto i2 = other->_fields.begin();

    for ( ; i1 != _fields.end(); ++i1, ++i2 ) {
        auto f1 = *i1;
        auto f2 = *i2;

        if ( f1->id()->name() != f2->id()->name() )
            return false;

        if ( ! f1->type()->equal(f2->type()) )
            return false;

        if ( f1->default_() && ! f2->default_() )
            return false;

        if ( f2->default_() && ! f1->default_() )
            return false;

        // Comparing the expression by rendering them.
        if ( f1->default_() && f2->default_() ) {
            std::ostringstream e1;
            passes::Printer(e1, true).run(f1->default_());

            std::ostringstream e2;
            passes::Printer(e2, true).run(f1->default_());

            if ( e1.str() != e2.str() )
                return false;
        }
    }

    return true;
}

