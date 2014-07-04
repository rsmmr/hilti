
#include <initializer_list>

#include "expression.h"
#include "type.h"
#include "variable.h"
#include "function.h"
#include "builder/nodes.h"
#include "passes/printer.h"

using namespace hilti;

Type::Type(const Location& l) : ast::Type<AstInfo>(l)
{
    _attributes = std::make_shared<AttributeSet>();
    addChild(_attributes);
}

const AttributeSet& Type::attributes() const
{
    auto r = ast::tryCast<type::Reference>(const_cast<Type *>(this));

    if ( r )
        // Forward to main type.
        return r->argType()->attributes();
    else
        return *_attributes;
}


AttributeSet& Type::attributes()
{
    auto r = ast::tryCast<type::Reference>(this);

    if ( r )
        // Forward to main type.
        return r->argType()->attributes();
    else
        return *_attributes;
}

void Type::setAttributes(const AttributeSet& attrs)
{
    auto r = ast::tryCast<type::Reference>(this);

    if ( r )
        // Forward to main type.
        r->argType()->setAttributes(attrs);
    else
        *_attributes = attrs;
}

string Type::render()
{
    std::ostringstream s;
    passes::Printer p(s, true);
    p.setQualifyTypeIDs(true);
    p.run(sharedPtr<Node>());
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
type::Context::~Context() {}
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
type::Scope::~Scope() {}
type::Time::~Time() {}
type::Timer::~Timer() {}
type::TimerMgr::~TimerMgr() {}
type::TypeType::~TypeType() {}
type::TypeByName::~TypeByName() {}
type::Unknown::~Unknown() {}
type::Unset::~Unset() {}
type::Vector::~Vector() {}
type::Void::~Void() {}

type::HiltiType::HiltiType(const Location& l) : Type(l)
{
}

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

type::Function::Function(shared_ptr<hilti::type::function::Result> result, const function::parameter_list& args, hilti::type::function::CallingConvention cc, const Location& l)
    : HiltiType(l), ast::type::mixin::Function<AstInfo>(this, result, args)
{
    _cc = cc;
    _plusone = (cc == function::CallingConvention::HILTI || cc == function::CallingConvention::CALLABLE);
}

type::Function::Function(const Location& l)
    : HiltiType(l),
    ast::type::mixin::Function<AstInfo>(this, std::make_shared<function::Result>(std::make_shared<Void>(), false, l), parameter_list())
{
    setWildcard(true);
    _cc = function::CallingConvention::HILTI;
}

bool type::Function::mayTriggerSafepoint() const
{
    if ( attributes().has(attribute::SAFEPOINT) )
        return true;

    return mayYield();
}

bool type::Function::mayYield() const
{
    switch ( _cc ) {
     case function::HILTI:
     case function::HOOK:
        // Default is may yield.
        return ! attributes().has(attribute::NOYIELD);

     case function::HILTI_C:
        // Default is no yield.
        return attributes().has(attribute::MAYYIELD);

     case function::C:
        return false;

     case function::CALLABLE:
        assert(false);
        return false;

     default:
        assert(false);
        return false;
    }
}

type::HiltiFunction::~HiltiFunction()
{
}

type::function::Parameter::Parameter(shared_ptr<hilti::ID> id, shared_ptr<Type> type, bool constant, shared_ptr<Expression> default_value, Location l)
    : ast::type::mixin::function::Parameter<AstInfo>(id, type, constant, default_value, l)
{
}

type::function::Result::Result(shared_ptr<Type> type, bool constant, Location l)
    : ast::type::mixin::function::Result<AstInfo>(type, constant, l)
{
}

type::Hook::Hook(shared_ptr<hilti::type::function::Result> result, const function::parameter_list& args,
                 const Location& l) : Function(result, args, function::HOOK, l)
{
    setCcPlusOne(false); /* ccPplus is tricky for the linker. */
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

    if ( wildcard() )
        return params;

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

shared_ptr<hilti::Type> type::Map::elementType()
{
    builder::type_list ty = { _key, _value };
    return builder::tuple::type(ty);
}

type::Exception::Exception(shared_ptr<Type> base, shared_ptr<Type> arg, const Location& l) : TypedHeapType(arg, l)
{
    _base = base;
    addChild(_base);
}

type::trait::Parameterized::parameter_list type::Callable::parameters() const
{
    type::trait::Parameterized::parameter_list params;

    params.push_back(std::make_shared<trait::parameter::Type>(result()->type()));

    for ( auto p : Function::parameters() )
        params.push_back(std::make_shared<trait::parameter::Type>(p->type()));

    return params;
}

shared_ptr<Type> type::Bytes::iterType()
{
    return shared_ptr<Type>(new type::iterator::Bytes(location()));
}

shared_ptr<Type> type::Bytes::elementType()
{
    return builder::bytes::type();
}

shared_ptr<Type> type::IOSource::iterType()
{
    return std::make_shared<iterator::IOSource>(sharedPtr<Type>(), location());
}

shared_ptr<Type> type::IOSource::elementType()
{
    builder::type_list elems = { builder::time::type(), builder::reference::type(builder::bytes::type()) };
    return builder::tuple::type(elems);
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
    { "BytesRunLength", std::make_shared<type::TypeByName>("Hilti::Packed"), nullptr, false,
      "A series of bytes preceded by an uint indicating its length." },

    { "BytesFixed", std::make_shared<type::Integer>(64), nullptr, false,
      "A series of bytes of fixed length specified by an additional integer argument" },

    { "BytesFixedOrEod", std::make_shared<type::Integer>(64), nullptr, false,
      "A series of bytes of fixed length specified by an additional integer argument, or until end-of-data if the input is frozen, whatever comes first." },

    { "BytesDelim", std::make_shared<type::Reference>(std::make_shared<type::Bytes>()), nullptr, false,
      "A series of bytes delimited by a final byte-sequence specified by an additional argument." },

    { "SkipBytesRunLength", std::make_shared<type::TypeByName>("Hilti::Packed"), nullptr, false,
      "Like BytesRunLength, but does not return unpacked value." },

    { "SkipBytesFixedOrEod", std::make_shared<type::Integer>(64), nullptr, false,
      "Like BytesFixed, but does not return unpacked value." },

    { "SkipBytesDelim", std::make_shared<type::Reference>(std::make_shared<type::Bytes>()), nullptr, false,
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

    _labels.sort([] (const Label& lhs, const Label& rhs) { return lhs.first->name().compare(rhs.first->name()) < 0; });
}

shared_ptr<hilti::Scope> type::Bitset::typeScope()
{
    if ( _scope )
        return _scope;

    _scope = shared_ptr<hilti::Scope>(new hilti::Scope());

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
    _labels.sort([] (const Label& lhs, const Label& rhs) { return lhs.first->name().compare(rhs.first->name()) < 0; });
}

shared_ptr<hilti::Scope> type::Enum::typeScope()
{
    if ( _scope )
        return _scope;

    _scope = shared_ptr<hilti::Scope>(new hilti::Scope());

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
        if ( l.first->name() == label->name() )
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
        if ( i1->first->name() != i2->first->name() || i1->second != i2->second )
            return false;
    }

    return true;
}

bool type::Scope::_equal(shared_ptr<Type> other) const
{
    auto sother = std::dynamic_pointer_cast<type::Scope>(other);
    assert(sother);

    if ( _fields.size() != sother->_fields.size() )
        return false;

    for ( auto p : ::util::zip2(_fields, sother->_fields) ) {
        if ( p.first != p.second )
            return false;
    }

    return true;
}

type::struct_::Field::Field(shared_ptr<ID> id, shared_ptr<hilti::Type> type, bool internal, const Location& l)
    : Node(l), _id(id), _type(type), _internal(internal)
{
    addChild(_id);
    addChild(_type);
}

shared_ptr<Expression> type::struct_::Field::default_() const
{
    return _type->attributes().getAsExpression(attribute::DEFAULT);
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

void type::Struct::addField(shared_ptr<struct_::Field> field)
{
    _fields.push_back(field);
    addChild(_fields.back());
}

shared_ptr<type::struct_::Field> type::Struct::lookup(shared_ptr<ID> id) const
{
    for ( auto f : _fields ) {
        if ( f->id()->name() == id->name() )
            return f;
    }

    return nullptr;
}

type::Struct::field_list type::Struct::sortedFields()
{
    field_list sorted = _fields;

    sorted.sort([] (shared_ptr<struct_::Field> lhs, shared_ptr<struct_::Field> rhs) {
        return lhs->id()->name().compare(rhs->id()->name()) < 0; });

    return sorted;
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
    // Comparing the types by rendering them to avoid infinite recursion
    // for cycles.
    return const_cast<type::Struct *>(this)->render() == ty->render();

#if 0
    // This version has problems when some type IDs come without namespace.
    // But looks like can just render directly.

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

        // Comparing the types by rendering them to avoid infinite recursion
        // for cycles.
        std::ostringstream t1;
        passes::Printer(t1, true).run(f1->type());

        std::ostringstream t2;
        passes::Printer(t2, true).run(f2->type());

        if ( t1.str() != t2.str() )
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
            passes::Printer(e2, true).run(f2->default_());

            if ( e1.str() != e2.str() )
                return false;
        }
    }

    return true;
#endif
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
    { "Int8", std::make_shared<type::Tuple>(), nullptr, true, "8-bit signed integer in host byte order." },
    { "Int16", std::make_shared<type::Tuple>(), nullptr, true, "16-bit signed integer in host byte order." },
    { "Int32", std::make_shared<type::Tuple>(), nullptr, true, "32-bit signed integer in host byte order." },
    { "Int64", std::make_shared<type::Tuple>(), nullptr, true, "64-bit signed integer in host byte order." },
    { "Int8Big", std::make_shared<type::Tuple>(), nullptr, true, "8-bit signed integer in big-endian byte order." },
    { "Int16Big", std::make_shared<type::Tuple>(), nullptr, true, "16-bit signed integer in big-endian byte order." },
    { "Int32Big", std::make_shared<type::Tuple>(), nullptr, true, "32-bit signed integer in big-endian byte order." },
    { "Int64Big", std::make_shared<type::Tuple>(), nullptr, true, "64-bit signed integer in big-endian byte order." },
    { "Int8Little", std::make_shared<type::Tuple>(), nullptr, true, "8-bit signed integer in little-endian byte order." },
    { "Int16Little", std::make_shared<type::Tuple>(), nullptr, true, "16-bit signed integer in little-endian byte order." },
    { "Int32Little", std::make_shared<type::Tuple>(), nullptr, true, "32-bit signed integer in little-endian byte order." },
    { "Int64Little", std::make_shared<type::Tuple>(), nullptr, true, "64-bit signed integer in little-endian byte order." },
    { "UInt8", std::make_shared<type::Tuple>(), nullptr, true, "8-bit unsigned integer in host byte order." },
    { "UInt16", std::make_shared<type::Tuple>(), nullptr, true, "16-bit unsigned integer in host byte order." },
    { "UInt32", std::make_shared<type::Tuple>(), nullptr, true, "32-bit unsigned integer in host byte order." },
    { "UInt64", std::make_shared<type::Tuple>(), nullptr, true, "64-bit unsigned integer in host byte order." },
    { "UInt8Big", std::make_shared<type::Tuple>(), nullptr, true, "8-bit unsigned integer in big-endian byte order." },
    { "UInt16Big", std::make_shared<type::Tuple>(), nullptr, true, "16-bit unsigned integer in big-endian byte order." },
    { "UInt32Big", std::make_shared<type::Tuple>(), nullptr, true, "32-bit unsigned integer in big-endian byte order." },
    { "UInt64Big", std::make_shared<type::Tuple>(), nullptr, true, "64-bit unsigned integer in big-endian byte order." },
    { "UInt8Little", std::make_shared<type::Tuple>(), nullptr, true, "8-bit unsigned integer in little-endian byte order." },
    { "UInt16Little", std::make_shared<type::Tuple>(), nullptr, true, "16-bit unsigned integer in little-endian byte order." },
    { "UInt32Little", std::make_shared<type::Tuple>(), nullptr, true, "32-bit unsigned integer in little-endian byte order." },
    { "UInt64Little", std::make_shared<type::Tuple>(), nullptr, true, "64-bit unsigned integer in little-endian byte order." }
};

std::vector<type::trait::Unpackable::Format> unpack_formats_integer(makeVector(_unpack_formats_integer));

const std::vector<type::trait::Unpackable::Format>& type::Integer::unpackFormats() const
{
    return unpack_formats_integer;
}

type::trait::Unpackable::Format _unpack_formats_addr[] = {
    { "IPv4", std::make_shared<type::TypeByName>("Hilti::Packed"), nullptr, false,
      "32-bit IPv4 address stored in host byte order." },

    { "IPv4Network", std::make_shared<type::TypeByName>("Hilti::Packed"), nullptr, false,
      "32-bit IPv4 address stored in network byte order." },

    { "IPv6", std::make_shared<type::TypeByName>("Hilti::Packed"), nullptr, false,
      "128-bit IPv6 address stored in host byte order." },

    { "IPv6Network", std::make_shared<type::TypeByName>("Hilti::Packed"), nullptr, false,
      "128-bit IPv6 address stored in network byte order." },
};

std::vector<type::trait::Unpackable::Format> unpack_formats_addr(makeVector(_unpack_formats_addr));

const std::vector<type::trait::Unpackable::Format>& type::Address::unpackFormats() const
{
    return unpack_formats_addr;
}

type::trait::Unpackable::Format _unpack_formats_port[] = {
    { "PortTCP", std::make_shared<type::TypeByName>("Hilti::Packed"), nullptr, false,
      "A 16-bit TCP port value stored in host byte order." },

    { "PortTCPNetwork", std::make_shared<type::TypeByName>("Hilti::Packed"), nullptr, false,
      "A 16-bit TCP port value stored in network byte order." },

    { "PortUDP", std::make_shared<type::TypeByName>("Hilti::Packed"), nullptr, false,
      "A 16-bit UDP port value stored in host byte order." },

    { "PortUDPNetwork", std::make_shared<type::TypeByName>("Hilti::Packed"), nullptr, false,
      "A 16-bit UDP port value stored in network byte order." },
};

std::vector<type::trait::Unpackable::Format> unpack_formats_port(makeVector(_unpack_formats_port));

const std::vector<type::trait::Unpackable::Format>& type::Port::unpackFormats() const
{
    return unpack_formats_port;
}

type::trait::Unpackable::Format _unpack_formats_bool[] = {
    { "Bool", std::make_shared<type::TypeByName>("Hilti::Packed"), nullptr, false,
      R"(A single bytes and, per default, returns ``True` if that byte is non-zero
        and ``False`` otherwise. Optionally, one can specify a particular bit
        (0-7) as additional ``unpack`` arguments and the result will then
        reflect the value of that bit.)" }
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
    if ( wildcard() )
        return parameter_list();

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

type::Context::Context(const field_list& fields, const Location& l)
    : Struct(fields, l) // Sort the fields so that order doesn't matter for hashing.
{
}

bool type::Scope::hasField(shared_ptr<ID> id) const
{
    for ( auto f : _fields ) {
        if ( id->name() == f->name() )
            return true;
    }

    return false;
}
