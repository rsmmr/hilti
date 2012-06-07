
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
type::TypeType::~TypeType() {}
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
    for ( auto t : types )
        _types.push_back(t);

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

const type::trait::TypeList::type_list type::Tuple::typeList() const
{
    type::trait::TypeList::type_list types;

    for ( auto t : _types )
        types.push_back(t);

    return types;
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
    parameter_list params;

    if ( wildcard() )
        return params;

    auto key = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(_key));
    auto value = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(_value));

    params.push_back(key);
    params.push_back(value);

    return params;
}

type::Exception::Exception(shared_ptr<Type> base, shared_ptr<Type> arg, const Location& l) : TypedHeapType(arg, l)
{
    _base = base;
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
    { "BytesRunLength", std::make_shared<type::TypeByName>("Hilti::Packed"), false,
      "A series of bytes preceded by an uint indicating its length." },

    { "BytesFixed", std::make_shared<type::Integer>(64), false,
      "A series of bytes of fixed length specified by an additional integer argument" },

    { "BytesDelim", std::make_shared<type::Reference>(std::make_shared<type::Bytes>()), false,
      "A series of bytes delimited by a final byte-sequence specified by an additional argument." },

    { "SkipBytesRunLength", std::make_shared<type::TypeByName>("Hilti::Packed"), false,
      "Like BytesRunLength, but does not return unpacked value." },

    { "SkipBytesFixed", std::make_shared<type::Integer>(64), false,
      "Like BytesFixed, but does not return unpacked value." },

    { "SkipBytesDelim", std::make_shared<type::Reference>(std::make_shared<type::Bytes>()), false,
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
    uint64_t flags = 0;

    for ( auto a : _attrs ) {
        if ( a == "&nosub" )
            flags |= 1;
        else {
            fprintf(stderr, "unknown regexp attribute '%s'", a.c_str());
            abort();
        }
    }

    parameter_list params = { shared_ptr<trait::parameter::Base>(new trait::parameter::Integer(flags)) };
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

type::Struct::Struct(const Location& l) : HeapType(l)
{
    setWildcard(true);
}

type::Struct::Struct(const field_list& fields, const Location& l) : HeapType(l)
{
    _fields = fields;

    for ( auto f : _fields )
        addChild(f);
}

const type::trait::TypeList::type_list type::Struct::typeList() const
{
    type::trait::TypeList::type_list types;

    for ( auto f : _fields )
        types.push_back(f->type());

    return types;
}

type::trait::Parameterized::parameter_list type::Struct::parameters() const
{
    type::trait::Parameterized::parameter_list params;

    for ( auto f : _fields ) {
        auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(f->type()));
        params.push_back(p);
    }

    return params;
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

bool type::Function::_equal(shared_ptr<hilti::Type> o) const
{
    auto other = ast::as<type::Function>(o);

    if ( ((*result()) != (*other->result())) )
        return false;

    auto p1 = parameters();
    auto p2 = other->parameters();

    if ( p1.size() != p2.size() )
        return false;

    auto i = p1.begin();
    auto j = p2.begin();

    while ( i != p1.end() ) {
        if ( *(*i++) != *(*j++) )
            return false;
    }

    return true;
}

type::trait::Unpackable::Format _unpack_formats_integer[] = {
    { "Int8", std::make_shared<type::Tuple>(), true, "8-bit signed integer in host byte order." },
    { "Int16", std::make_shared<type::Tuple>(), true, "16-bit signed integer in host byte order." },
    { "Int32", std::make_shared<type::Tuple>(), true, "32-bit signed integer in host byte order." },
    { "Int64", std::make_shared<type::Tuple>(), true, "64-bit signed integer in host byte order." },
    { "Int8Big", std::make_shared<type::Tuple>(), true, "8-bit signed integer in big-endian byte order." },
    { "Int16Big", std::make_shared<type::Tuple>(), true, "16-bit signed integer in big-endian byte order." },
    { "Int32Big", std::make_shared<type::Tuple>(), true, "32-bit signed integer in big-endian byte order." },
    { "Int64Big", std::make_shared<type::Tuple>(), true, "64-bit signed integer in big-endian byte order." },
    { "Int8Little", std::make_shared<type::Tuple>(), true, "8-bit signed integer in little-endian byte order." },
    { "Int16Little", std::make_shared<type::Tuple>(), true, "16-bit signed integer in little-endian byte order." },
    { "Int32Little", std::make_shared<type::Tuple>(), true, "32-bit signed integer in little-endian byte order." },
    { "Int64Little", std::make_shared<type::Tuple>(), true, "64-bit signed integer in little-endian byte order." },
    { "UInt8", std::make_shared<type::Tuple>(), true, "8-bit unsigned integer in host byte order." },
    { "UInt16", std::make_shared<type::Tuple>(), true, "16-bit unsigned integer in host byte order." },
    { "UInt32", std::make_shared<type::Tuple>(), true, "32-bit unsigned integer in host byte order." },
    { "UInt64", std::make_shared<type::Tuple>(), true, "64-bit unsigned integer in host byte order." },
    { "UInt8Big", std::make_shared<type::Tuple>(), true, "8-bit unsigned integer in big-endian byte order." },
    { "UInt16Big", std::make_shared<type::Tuple>(), true, "16-bit unsigned integer in big-endian byte order." },
    { "UInt32Big", std::make_shared<type::Tuple>(), true, "32-bit unsigned integer in big-endian byte order." },
    { "UInt64Big", std::make_shared<type::Tuple>(), true, "64-bit unsigned integer in big-endian byte order." },
    { "UInt8Little", std::make_shared<type::Tuple>(), true, "8-bit unsigned integer in little-endian byte order." },
    { "UInt16Little", std::make_shared<type::Tuple>(), true, "16-bit unsigned integer in little-endian byte order." },
    { "UInt32Little", std::make_shared<type::Tuple>(), true, "32-bit unsigned integer in little-endian byte order." },
    { "UInt64Little", std::make_shared<type::Tuple>(), true, "64-bit unsigned integer in little-endian byte order." }
};

std::vector<type::trait::Unpackable::Format> unpack_formats_integer(makeVector(_unpack_formats_integer));

const std::vector<type::trait::Unpackable::Format>& type::Integer::unpackFormats() const
{
    return unpack_formats_integer;
}

type::trait::Unpackable::Format _unpack_formats_addr[] = {
    { "IPv4", std::make_shared<type::TypeByName>("Hilti::Packed"), false,
      "32-bit IPv4 address stored in host byte order." },

    { "IPv4Network", std::make_shared<type::TypeByName>("Hilti::Packed"), false,
      "32-bit IPv4 address stored in network byte order." },

    { "IPv6", std::make_shared<type::TypeByName>("Hilti::Packed"), false,
      "128-bit IPv6 address stored in host byte order." },

    { "IPv6Network", std::make_shared<type::TypeByName>("Hilti::Packed"), false,
      "128-bit IPv6 address stored in network byte order." },
};

std::vector<type::trait::Unpackable::Format> unpack_formats_addr(makeVector(_unpack_formats_addr));

const std::vector<type::trait::Unpackable::Format>& type::Address::unpackFormats() const
{
    return unpack_formats_addr;
}

type::trait::Unpackable::Format _unpack_formats_port[] = {
    { "PortTCP", std::make_shared<type::TypeByName>("Hilti::Packed"), false,
      "A 16-bit TCP port value stored in host byte order." },

    { "PortTCPNetwork", std::make_shared<type::TypeByName>("Hilti::Packed"), false,
      "A 16-bit TCP port value stored in network byte order." },

    { "PortUDP", std::make_shared<type::TypeByName>("Hilti::Packed"), false,
      "A 16-bit UDP port value stored in host byte order." },

    { "PortUDPNetwork", std::make_shared<type::TypeByName>("Hilti::Packed"), false,
      "A 16-bit UDP port value stored in network byte order." },
};

std::vector<type::trait::Unpackable::Format> unpack_formats_port(makeVector(_unpack_formats_port));

const std::vector<type::trait::Unpackable::Format>& type::Port::unpackFormats() const
{
    return unpack_formats_port;
}

type::trait::Unpackable::Format _unpack_formats_bool[] = {
    { "Bool", std::make_shared<type::TypeByName>("Hilti::Packed"), false,
      "A single bytes." }
};

std::vector<type::trait::Unpackable::Format> unpack_formats_bool(makeVector(_unpack_formats_bool));

const std::vector<type::trait::Unpackable::Format>& type::Bool::unpackFormats() const
{
    return unpack_formats_bool;
}

type::Classifier::Classifier(shared_ptr<Type> rtype, shared_ptr<Type> vtype, const Location& l)
    : HeapType(l)
{
    _rtype = rtype;
    _vtype = vtype;
    addChild(_rtype);
    addChild(_vtype);
}

type::Classifier::Classifier(const Location& l)
{
    setWildcard(true);
}

type::trait::Parameterized::parameter_list type::Classifier::parameters() const
{
    auto rtype = std::make_shared<trait::parameter::Type>(_rtype);
    auto vtype = std::make_shared<trait::parameter::Type>(_vtype);

    parameter_list params = { rtype, vtype };
    return params;
}

bool type::Classifier::_equal(shared_ptr<hilti::Type> t) const
{
    auto other = ast::as<type::Classifier>(t);
    assert(other);

    return _rtype->equal(other->_rtype) && _vtype->equal(other->_vtype);
}

type::overlay::Field::Field(shared_ptr<ID> name, shared_ptr<Type> type, shared_ptr<ID> start, shared_ptr<Expression> fmt, shared_ptr<Expression> arg, const Location& l)
    : Node(l)
{
    _name = name;
    _start_offset = -1;
    _start_field = start;
    _type = type;
    _fmt = fmt;
    _fmt_arg = arg;
    _idx = -1;

    addChild(_name);
    addChild(_type);
    addChild(_start_field);
    addChild(_fmt);
    addChild(_fmt_arg);
}

type::overlay::Field::Field(shared_ptr<ID> name, shared_ptr<Type> type, int start, shared_ptr<Expression> fmt, shared_ptr<Expression> arg, const Location& l)
    : Node(l)
{
    _name = name;
    _start_offset = start;
    _start_field = nullptr;
    _type = type;
    _fmt = fmt;
    _fmt_arg = arg;
    _idx = -1;

    addChild(_name);
    addChild(_type);
    addChild(_start_field);
    addChild(_fmt);
    addChild(_fmt_arg);
}

type::overlay::Field::~Field()
{
    // Nothing to do.
}

shared_ptr<Expression> type::overlay::Field::format() const
{
    return _fmt;
}

shared_ptr<Expression> type::overlay::Field::formatArg() const
{
    return _fmt_arg;
}

bool type::overlay::Field::equal(shared_ptr<Field> other) const
{
    // We can't compare the expressions, so let's always say no.
    return false;

#if 0
    return _name->name() == other->_name->name()
        && _start_offset = other->_start_offset
        && ( (! _start_field && ! other->_start_field) || _start_field->name() == other->_start_field->name())
        && _type->equal(other->_type)
        && _fmt->equal(other->_fmt)
        && (! _arg || _arg->equal(other->_arg));
#endif
}

type::Overlay::Overlay(const field_list& fields, const Location& l)
    : ValueType(l)
{
    _fields = fields;

    for ( auto f : _fields )
        addChild(f);

    Init();
}

type::Overlay::Overlay(const Location& l) : ValueType(l)
{
    setWildcard(true);
}

void type::Overlay::Init()
{
    for ( auto f : _fields ) {
        if ( ! f->_start_field )
            continue;

        // Field depends on another one.
        auto dep = field(f->_start_field);
        f->_deps = dep->_deps;
        f->_deps.push_back(dep);

        if ( dep->_idx < 0)
            // Dep fields needs an index for caching.
            dep->_idx = ++_idxcnt;
    }
}

shared_ptr<type::overlay::Field> type::Overlay::field(const string& name) const
{
    for ( auto f : _fields ) {
        if ( f->name()->name() == name )
            return f;
    }

    return nullptr;
}

shared_ptr<type::overlay::Field> type::Overlay::field(shared_ptr<ID> name) const
{
    for ( auto f : _fields ) {
        if ( f->name()->name() == name->name() )
            return f;
    }

    return nullptr;
}

bool type::Overlay::_equal(shared_ptr<hilti::Type> t) const
{
    auto other = ast::as<type::Overlay>(t);
    assert(other);

    if ( _fields.size() != other->_fields.size() )
        return false;

    auto f1 = _fields.begin();
    auto f2 = other->_fields.begin();

    while ( f1 != _fields.end() ) {
        if ( ! (*f1++)->equal(*f2) )
            return false;
    }

    return true;
}
