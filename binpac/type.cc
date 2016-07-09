

#include "type.h"
#include "attribute.h"
#include "constant.h"
#include "declaration.h"
#include "expression.h"
#include "grammar.h"
#include "passes/printer.h"
#include "scope.h"
#include "statement.h"

using namespace binpac;
using namespace type;

trait::Parseable::~Parseable()
{
}

shared_ptr<binpac::Type> trait::Parseable::fieldType()
{
    auto t = dynamicCast(this, binpac::Type*);
    assert(t);
    return t->sharedPtr<binpac::Type>();
}

std::list<trait::Parseable::ParseAttribute> trait::Parseable::parseAttributes() const
{
    return {};
}

string binpac::Type::render()
{
    std::ostringstream s;
    passes::Printer p(s, true);
    p.setQualifyTypeIDs(true);
    p.run(sharedPtr<Node>());
    return s.str();
}

binpac::Type::Type(const Location& l) : ast::Type<AstInfo>(l)
{
}

shared_ptr<Scope> binpac::Type::typeScope()
{
    return nullptr;
}

OptionalArgument::OptionalArgument(shared_ptr<Type> arg)
{
    _arg = arg;
}

shared_ptr<binpac::Type> OptionalArgument::argType() const
{
    return _arg;
}

bool OptionalArgument::equal(shared_ptr<binpac::Type> other) const
{
    return other ? _arg->equal(other) : true;
}

string OptionalArgument::render()
{
    return string("[ " + _arg->render() + " ]");
}

string binpac::Type::docRender()
{
    return render();
}

PacType::PacType(const Location& l) : binpac::Type(l)
{
    _attrs = std::make_shared<AttributeSet>(l);
}

PacType::PacType(const attribute_list& attrs, const Location& l) : binpac::Type(l)
{
    _attrs = std::make_shared<AttributeSet>(attrs, l);
}

shared_ptr<AttributeSet> PacType::attributes() const
{
    return _attrs;
}

TypedPacType::TypedPacType(const Location& l) : PacType(l)
{
    setWildcard(true);
}

TypedPacType::TypedPacType(shared_ptr<Type> rtype, const Location& l) : PacType(l)
{
    _argtype = rtype;
    addChild(_argtype);
}

shared_ptr<Type> TypedPacType::argType() const
{
    return _argtype;
}

bool TypedPacType::_equal(shared_ptr<binpac::Type> other) const
{
    return trait::Parameterized::equal(other);
}

trait::Parameterized::type_parameter_list TypedPacType::parameters() const
{
    type_parameter_list params;

    if ( ! _argtype )
        return params;

    auto p = std::make_shared<trait::parameter::Type>(_argtype);
    params.push_back(p);
    return params;
}

Iterator::Iterator(const Location& l) : TypedPacType(l)
{
}

Iterator::Iterator(shared_ptr<Type> ttype, const Location& l) : TypedPacType(ttype, l)
{
}

bool Iterator::equal(shared_ptr<Type> other) const
{
    // Allow matching of derived classes against an iterator wildcard.
    if ( ast::tryCast<Iterator>(other) && (wildcard() || other->wildcard()) )
        return true;

    return trait::Parameterized::equal(other);
}

Any::Any(const Location& l) : binpac::Type(l)
{
    setMatchesAny(true);
}


Unknown::Unknown(const Location& l) : binpac::Type(l)
{
}

Unknown::Unknown(shared_ptr<ID> id, const Location& l) : binpac::Type(l)
{
    _id = id;
    addChild(_id);
}

shared_ptr<ID> Unknown::id() const
{
    return _id;
}

UnknownElementType::UnknownElementType(node_ptr<Expression> expr, const Location& l)
    : binpac::Type(l)
{
    _expr = expr;
    addChild(_expr);
}

shared_ptr<Expression> UnknownElementType::expression() const
{
    return _expr;
}

TypeByName::TypeByName(shared_ptr<ID> id, const Location& l) : binpac::Type(l)
{
    _id = id;
}

TypeByName::TypeByName(const string& id, const Location& l)
{
    _id = std::make_shared<ID>(id, l);
}

const shared_ptr<ID> TypeByName::id() const
{
    return _id;
}

bool TypeByName::_equal(shared_ptr<binpac::Type> other) const
{
    return dynamicPointerCast(other)->_id->name() == _id->name(, TypeByName);
}

Unset::Unset(const Location& l) : PacType(l)
{
}

MemberAttribute::MemberAttribute(shared_ptr<ID> attr, const Location& l) : binpac::Type(l)
{
    _attribute = attr;
    addChild(_attribute);
}

MemberAttribute::MemberAttribute(const Location& l) : binpac::Type(l)
{
    setWildcard(true);
}

shared_ptr<ID> MemberAttribute::attribute() const
{
    return _attribute;
}

string MemberAttribute::render()
{
    return _attribute ? _attribute->name() : "<id>";
}

bool MemberAttribute::_equal(shared_ptr<binpac::Type> other) const
{
    auto mother = dynamicPointerCast(other, MemberAttribute);
    assert(mother);

    return _attribute && mother->_attribute ? (*_attribute == *mother->_attribute) : true;
}

type::Module::Module(const Location& l) : binpac::Type(l)
{
}

Void::Void(const Location& l) : binpac::Type(l)
{
}


String::String(const Location& l) : PacType(l)
{
}


Address::Address(const Location& l) : PacType(l)
{
}

std::list<trait::Parseable::ParseAttribute> Address::parseAttributes() const
{
    return {{"byteorder",
             std::make_shared<type::TypeByName>(std::make_shared<ID>("BinPAC::ByteOrder")), nullptr,
             false},
            {"ipv4", nullptr, nullptr, false},
            {"ipv6", nullptr, nullptr, false}};
}

Network::Network(const Location& l) : PacType(l)
{
}

Port::Port(const Location& l) : PacType(l)
{
}

Bitset::Bitset(const label_list& labels, const Location& l) : PacType(l)
{
    int next = 0;
    for ( auto label : labels ) {
        auto bit = label.second;

        if ( bit < 0 )
            bit = next;

        next = std::max(next, bit + 1);

        _labels.push_back(make_pair(label.first, bit));
    }

    _labels.sort([](const Label& lhs, const Label& rhs) {
        return lhs.first->name().compare(rhs.first->name()) < 0;
    });
}

Bitset::Bitset(const Location& l) : PacType(l)
{
    setWildcard(true);
}

const Bitset::label_list& Bitset::labels() const
{
    return _labels;
}

int Bitset::labelBit(shared_ptr<ID> label) const
{
    for ( auto l : _labels ) {
        if ( *l.first == *label )
            return l.second;
    }

    throw ast::InternalError(util::fmt("unknown bitset label %s", label->pathAsString().c_str()),
                             this);
}

shared_ptr<Scope> Bitset::typeScope()
{
    if ( _scope )
        return _scope;

    _scope = shared_ptr<Scope>(new Scope());

    for ( auto label : _labels ) {
        auto p = shared_from_this();
        auto p2 = dynamicPointerCast(p, binpac::Type);
        constant::Bitset::bit_list bl;
        bl.push_back(label.first);
        auto val = shared_ptr<Constant>(new constant::Bitset(bl, p2, location()));
        auto expr = shared_ptr<expression::Constant>(new expression::Constant(val, location()));
        _scope->insert(label.first, expr);
    }

    return _scope;
}

bool Bitset::_equal(shared_ptr<Type> other) const
{
    auto bother = dynamicPointerCast(other, Bitset);
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

bitfield::Bits::Bits(shared_ptr<ID> id, int lower, int upper, int parent_width,
                     const attribute_list& attrs, const Location& l)
{
    _id = id;
    _attrs = std::make_shared<AttributeSet>(attrs);
    _lower = lower;
    _upper = upper;
    _parent_width = parent_width;

    addChild(_id);
    addChild(_attrs);
}

shared_ptr<ID> bitfield::Bits::id() const
{
    return _id;
}


int bitfield::Bits::lower() const
{
    return _lower;
}


int bitfield::Bits::upper() const
{
    return _upper;
}

shared_ptr<AttributeSet> bitfield::Bits::attributes() const
{
    return _attrs;
}

shared_ptr<Type> bitfield::Bits::fieldType() const
{
    auto convert = _attrs->lookup("convert");

    if ( convert )
        return convert->value()->type();

    else
        return std::make_shared<type::Integer>(_parent_width, false);
}

Bitfield::Bitfield(int width, const bits_list& bits, const Location& l)
{
    _width = width;

    setBits(bits);
    setBitOrder(std::make_shared<expression::ID>(std::make_shared<ID>("BinPAC::BitOrder::LSB0")));
}

Bitfield::Bitfield(const Location& l) : PacType(l)
{
    _width = 0;
    setWildcard(true);
}

int Bitfield::width() const
{
    return _width;
}

void Bitfield::setBits(const bits_list& bits)
{
    for ( auto b : _bits )
        removeChild(b);

    _bits.clear();

    for ( auto b : bits )
        _bits.push_back(b);

    for ( auto b : _bits )
        addChild(b);
}

Bitfield::bits_list Bitfield::bits() const
{
    bits_list bits;
    for ( auto b : _bits )
        bits.push_back(b);

    return bits;
}

shared_ptr<bitfield::Bits> Bitfield::bits(shared_ptr<ID> id) const
{
    for ( auto b : _bits ) {
        if ( *b->id() == *id )
            return b;
    }

    return nullptr;
}

shared_ptr<Expression> Bitfield::bitOrder()
{
    return _bit_order;
}

void Bitfield::setBitOrder(shared_ptr<Expression> order)
{
    removeChild(_bit_order);
    _bit_order = order;
    addChild(_bit_order);
}

std::list<trait::Parseable::ParseAttribute> Bitfield::parseAttributes() const
{
    return {
        {"byteorder", std::make_shared<type::TypeByName>(std::make_shared<ID>("BinPAC::ByteOrder")),
         nullptr, false},
        {"bitorder", std::make_shared<type::TypeByName>(std::make_shared<ID>("BinPAC::BitOrder")),
         nullptr, false},
    };
}

trait::Parameterized::type_parameter_list Bitfield::parameters() const
{
    type_parameter_list params;
    auto p = std::make_shared<trait::parameter::Integer>(_width);
    params.push_back(p);
    return params;
}

Enum::Enum(const label_list& labels, const Location& l) : PacType(l)
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
    _labels.sort([](const Label& lhs, const Label& rhs) {
        return lhs.first->name().compare(rhs.first->name()) < 0;
    });
}

Enum::Enum(const Location& l) : PacType(l)
{
    setWildcard(true);
}

shared_ptr<Scope> Enum::typeScope()
{
    if ( _scope )
        return _scope;

    _scope = shared_ptr<Scope>(new Scope());

    for ( auto label : _labels ) {
        auto p = shared_from_this();
        auto p2 = dynamicPointerCast(p, binpac::Type);
        auto val = shared_ptr<Constant>(new constant::Enum(label.first, p2, location()));
        auto expr = shared_ptr<expression::Constant>(new expression::Constant(val, location()));
        _scope->insert(label.first, expr);
    }

    return _scope;
}

int Enum::labelValue(shared_ptr<ID> label) const
{
    for ( auto l : _labels ) {
        if ( *l.first == *label )
            return l.second;
    }

    throw ast::InternalError(util::fmt("unknown enum label %s", label->pathAsString().c_str()),
                             this);
    return -1;
}

bool Enum::_equal(shared_ptr<Type> other) const
{
    auto eother = dynamicPointerCast(other, Enum);
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

const Enum::label_list& Enum::labels() const
{
    return _labels;
}

CAddr::CAddr(const Location& l) : PacType(l)
{
}

Double::Double(const Location& l) : PacType(l)
{
}

std::list<trait::Parseable::ParseAttribute> Double::parseAttributes() const
{
    return {
        {"byteorder", std::make_shared<type::TypeByName>(std::make_shared<ID>("BinPAC::ByteOrder")),
         nullptr, false},
        {"precision", std::make_shared<type::TypeByName>(std::make_shared<ID>("BinPAC::Precision")),
         std::make_shared<expression::ID>(std::make_shared<ID>("BinPAC::Precision::Double")), true},
    };
}

Bool::Bool(const Location& l) : PacType(l)
{
}

Interval::Interval(const Location& l) : PacType(l)
{
}

Time::Time(const Location& l) : PacType(l)
{
}

Integer::Integer(int width, bool sign, const Location& l) : PacType(l)
{
    _width = width;
    _signed = sign;
}

Integer::Integer(const Location& l) : PacType(l)
{
    _width = 0;
    _signed = true; // Not relevant but make sure it's initialized.
    setWildcard(true);
}

int Integer::width() const
{
    return _width;
}

bool Integer::signed_() const
{
    return _signed;
}

bool Integer::_equal(shared_ptr<binpac::Type> other) const
{
    auto iother = ast::checkedCast<type::Integer>(other);

    if ( signed_() != iother->signed_() )
        return false;

    return trait::Parameterized::equal(other);
}

std::list<trait::Parseable::ParseAttribute> Integer::parseAttributes() const
{
    return {
        {"byteorder", std::make_shared<type::TypeByName>(std::make_shared<ID>("BinPAC::ByteOrder")),
         nullptr, false},
    };
}

shared_ptr<Integer> Integer::unsignedInteger(int width, const Location& l)
{
    return std::make_shared<Integer>(width, false, l);
}

shared_ptr<Integer> Integer::signedInteger(int width, const Location& l)
{
    return std::make_shared<Integer>(width, true, l);
}

trait::Parameterized::type_parameter_list Integer::parameters() const
{
    type_parameter_list params;
    auto p = std::make_shared<trait::parameter::Integer>(_width);
    params.push_back(p);
    return params;
}

Tuple::Tuple(const Location& l) : PacType(l)
{
    setWildcard(true);
}

Tuple::Tuple(const type_list& types, const Location& l) : PacType(l)
{
    for ( auto t : types )
        _types.push_back(t);

    for ( auto t : _types )
        addChild(t);
}

trait::Parameterized::type_parameter_list Tuple::parameters() const
{
    type_parameter_list params;

    for ( auto t : _types ) {
        auto p = std::make_shared<trait::parameter::Type>(t);
        params.push_back(p);
    }

    return params;
}

const trait::TypeList::type_list Tuple::typeList() const
{
    trait::TypeList::type_list types;

    for ( auto t : _types )
        types.push_back(t);

    return types;
}

bool Tuple::_equal(shared_ptr<binpac::Type> other) const
{
    return trait::Parameterized::equal(other);
}

TypeType::TypeType(shared_ptr<binpac::Type> type, const Location& l) : binpac::Type(l)
{
    _rtype = type;
    addChild(_rtype);
}

TypeType::TypeType(const Location& l) : binpac::Type()
{
    setWildcard(true);
}

shared_ptr<binpac::Type> TypeType::typeType() const
{
    return _rtype;
}

bool TypeType::_equal(shared_ptr<binpac::Type> other) const
{
    return _rtype->equal(dynamicPointerCast(other)->_rtype, TypeType);
}

Exception::Exception(shared_ptr<Type> base, shared_ptr<Type> arg, const Location& l)
    : TypedPacType(arg, l)
{
    _base = base;
    addChild(_base);
}

Exception::Exception(const Location& l) : TypedPacType(l)
{
}

shared_ptr<Type> Exception::baseType() const
{
    return _base;
}

void Exception::setBaseType(shared_ptr<Type> base)
{
    _base = base;
}

bool Exception::isRootType() const
{
    return _libtype == "Hilti::UnspecifiedException";
}

void Exception::setLibraryType(const string& name)
{
    _libtype = name;
}

string Exception::libraryType() const
{
    return _libtype;
}

type::Function::Function(shared_ptr<type::function::Result> result, const parameter_list& args,
                         type::function::CallingConvention cc, const Location& l)
    : binpac::Type(l), ast::type::mixin::Function<AstInfo>(this, result, args)
{
    _cc = cc;
}

type::Function::Function(const Location& l)
    : binpac::Type(l),
      ast::type::mixin::Function<
          AstInfo>(this, std::make_shared<function::Result>(std::make_shared<Void>(), false, l),
                   parameter_list())
{
    setWildcard(true);
}

type::function::CallingConvention type::Function::callingConvention() const
{
    return _cc;
}

void type::Function::setCallingConvention(type::function::CallingConvention cc)
{
    _cc = cc;
}

bool type::Function::_equal(shared_ptr<binpac::Type> o) const
{
    auto other = ast::checkedCast<Function>(o);

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

function::Parameter::Parameter(shared_ptr<binpac::ID> id, shared_ptr<Type> type, bool constant,
                               bool clear, shared_ptr<Expression> default_value, Location l)
    : ast::type::mixin::function::Parameter<AstInfo>(id, type, constant, default_value, l)
{
    _clear = clear;
}

bool function::Parameter::clear() const
{
    return _clear;
}

function::Result::Result(shared_ptr<Type> type, bool constant, Location l)
    : ast::type::mixin::function::Result<AstInfo>(type, constant, l)
{
}

type::Hook::Hook(shared_ptr<binpac::type::function::Result> result, const parameter_list& args,
                 const Location& l)
    : Function(result, args, type::function::BINPAC)
{
}

type::Hook::Hook(const Location& l) : Function(l)
{
}

Bytes::Bytes(const Location& l) : PacType(l)
{
}

shared_ptr<Type> Bytes::iterType()
{
    return shared_ptr<Type>(new iterator::Bytes(location()));
}

shared_ptr<binpac::Type> Bytes::elementType()
{
    return std::make_shared<type::Integer>(8, false, location());
}

std::list<trait::Parseable::ParseAttribute> Bytes::parseAttributes() const
{
    auto one =
        std::make_shared<expression::Constant>(std::make_shared<constant::Integer>(1, 64, false));

    return {
        {"chunked", std::make_shared<type::Integer>(64, false), one, false},
        {"until", std::make_shared<type::Bytes>(), nullptr, false},
        {"eod", nullptr, nullptr, false},
    };
}

iterator::Bytes::Bytes(const Location& l) : Iterator(shared_ptr<Type>(new type::Bytes(l)))
{
}

File::File(const Location& l) : PacType(l)
{
}

iterator::ContainerIterator::ContainerIterator(shared_ptr<Type> ctype, const Location& l)
    : Iterator(ctype)
{
}

iterator::Map::Map(shared_ptr<Type> ctype, const Location& l) : ContainerIterator(ctype, l)
{
}

iterator::Set::Set(shared_ptr<Type> ctype, const Location& l) : ContainerIterator(ctype, l)
{
}

iterator::Vector::Vector(shared_ptr<Type> ctype, const Location& l) : ContainerIterator(ctype, l)
{
}

iterator::List::List(shared_ptr<Type> ctype, const Location& l) : ContainerIterator(ctype, l)
{
}

List::List(shared_ptr<Type> etype, const Location& l) : TypedPacType(etype, l)
{
}

List::List(const Location& l) : TypedPacType(l)
{
}

shared_ptr<binpac::Type> List::iterType()
{
    return std::make_shared<iterator::List>(this->sharedPtr<Type>(), location());
}

shared_ptr<binpac::Type> List::elementType()
{
    return argType();
}

std::list<trait::Parseable::ParseAttribute> List::parseAttributes() const
{
    return {{"count", std::make_shared<type::Integer>(64, false), nullptr, false},
            {"until", std::make_shared<type::Bool>(), nullptr, false},
            {"until_including", std::make_shared<type::Bool>(), nullptr, false},
            {"while", std::make_shared<type::Bool>(), nullptr, false}};
}

Vector::Vector(shared_ptr<Type> etype, const Location& l) : TypedPacType(etype, l)
{
}

Vector::Vector(const Location& l) : TypedPacType(l)
{
}

shared_ptr<binpac::Type> Vector::iterType()
{
    return std::make_shared<iterator::Vector>(this->sharedPtr<Type>(), location());
}

shared_ptr<binpac::Type> Vector::elementType()
{
    return argType();
}

Set::Set(shared_ptr<Type> etype, const Location& l) : TypedPacType(etype, l)
{
}

Set::Set(const Location& l) : TypedPacType(l)
{
}

shared_ptr<binpac::Type> Set::iterType()
{
    return std::make_shared<iterator::Set>(this->sharedPtr<Type>(), location());
}

shared_ptr<binpac::Type> Set::elementType()
{
    return argType();
}

Map::Map(shared_ptr<Type> key, shared_ptr<Type> value, const Location& l) : PacType(l)
{
    _key = key;
    _value = value;
    addChild(_key);
    addChild(_value);
}

shared_ptr<binpac::Type> Map::iterType()
{
    return std::make_shared<iterator::Map>(this->sharedPtr<Type>(), location());
}

shared_ptr<binpac::Type> Map::elementType()
{
    type_list types = {_key, _value};
    return std::make_shared<Tuple>(types, location());
}

bool Map::_equal(shared_ptr<binpac::Type> other) const
{
    return trait::Parameterized::equal(other);
}

trait::Parameterized::type_parameter_list Map::parameters() const
{
    type_parameter_list params;

    if ( wildcard() )
        return params;

    auto key = std::make_shared<trait::parameter::Type>(_key);
    auto value = std::make_shared<trait::parameter::Type>(_value);

    params.push_back(key);
    params.push_back(value);

    return params;
}

RegExp::RegExp(const attribute_list& attrs, const Location& l) : PacType(l)
{
    _attrs = std::make_shared<AttributeSet>(attrs, l);
}

RegExp::RegExp(const Location& l) : PacType(l)
{
    setWildcard(true);
}

bool RegExp::_equal(shared_ptr<binpac::Type> other) const
{
    return trait::Parameterized::equal(other);
}

trait::Parameterized::type_parameter_list RegExp::parameters() const
{
    uint64_t flags = 0;

    if ( _attrs->has("&nosub") )
        flags = 1;

    type_parameter_list params = {std::make_shared<trait::parameter::Integer>(flags)};
    return params;
}

shared_ptr<binpac::Type> RegExp::fieldType()
{
    return std::make_shared<type::Bytes>();
}

TimerMgr::TimerMgr(const Location& l) : PacType(l)
{
}


Timer::Timer(const Location& l) : PacType(l)
{
}

int unit::Item::Item::_id_counter = 0;

unit::Item::Item(shared_ptr<ID> id, shared_ptr<Type> type, const hook_list& hooks,
                 const attribute_list& attrs, const Location& l)
    : Node(l)
{
    if ( ! id ) {
        id = std::make_shared<ID>(util::fmt("__anon%d", ++_id_counter), l);
        _anonymous = true;
    }

    _id = id;
    _type = type;
    _attrs = std::make_shared<AttributeSet>(attrs);

    addChild(_id);
    addChild(_type);
    addChild(_attrs);

    for ( auto h : hooks )
        _hooks.push_back(h);

    for ( auto h : _hooks )
        addChild(h);

    _scope = std::make_shared<Scope>(); // The scope builder will initialize  this and link it in.
}

shared_ptr<type::Unit> unit::Item::unit() const
{
    return _unit ? _unit->sharedPtr<type::Unit>() : nullptr;
}

void unit::Item::setUnit(type::Unit* unit)
{
    _unit = unit;
}

shared_ptr<ID> unit::Item::id() const
{
    return _id;
}

bool unit::Item::anonymous() const
{
    return _anonymous;
}

void unit::Item::setAnonymous()
{
    _anonymous = true;
}

bool unit::Item::aliased() const
{
    return _aliased;
}

void unit::Item::setAliased()
{
    _aliased = true;
}

void unit::Item::setCtorNoName()
{
    _ctor_no_name = true;
}

bool unit::Item::ctorNoName() const
{
    return _ctor_no_name;
}

shared_ptr<Type> unit::Item::type()
{
    return _type;
}

shared_ptr<Type> unit::Item::fieldType()
{
    auto parseable = ast::type::tryTrait<type::trait::Parseable>(type());

    if ( parseable )
        return parseable->fieldType();
    else
        return type();
}

shared_ptr<Scope> unit::Item::scope() const
{
    return _scope;
}

hook_list unit::Item::hooks() const
{
    hook_list hooks;

    for ( auto h : _hooks )
        hooks.push_back(h);

    return hooks;
}

void unit::Item::addHook(shared_ptr<binpac::Hook> hook)
{
    _hooks.push_back(hook);
    addChild(_hooks.back());
}

void unit::Item::setType(shared_ptr<binpac::Type> type)
{
    removeChild(_type);
    _type = type;
    addChild(_type);
}

shared_ptr<AttributeSet> unit::Item::attributes() const
{
    return _attrs;
}

shared_ptr<binpac::Expression> unit::Item::inheritedProperty(const string& property)
{
    if ( ! _unit )
        return nullptr;

    return _unit->inheritedProperty(property, this->sharedPtr<Item>());
}

void unit::Item::disableHooks()
{
    --_do_hooks;
}

void unit::Item::enableHooks()
{
    ++_do_hooks;
}

bool unit::Item::hooksEnabled()
{
    return _do_hooks != 0;
}

unit::item::Field::Field(shared_ptr<ID> id, shared_ptr<binpac::Type> type, Kind kind,
                         shared_ptr<Expression> cond, const hook_list& hooks,
                         const attribute_list& attrs, const expression_list& params,
                         const expression_list& sinks, const Location& l)
    : Item(id, type, hooks, attrs, l)
{
    _kind = kind;
    _cond = cond;
    addChild(_cond);

    for ( auto s : sinks )
        _sinks.push_back(s);

    for ( auto p : params )
        _params.push_back(p);

    for ( auto s : _sinks )
        addChild(s);

    for ( auto p : _params )
        addChild(p);
}

shared_ptr<Type> unit::item::Field::fieldType()
{
    auto ftype = Item::fieldType();

    auto convert = attributes()->lookup("convert");

    if ( convert )
        return convert->value()->type();

    return ftype;
}

shared_ptr<unit::item::Field> unit::item::Field::createByType(
    shared_ptr<Type> type, shared_ptr<ID> id, Kind kind, shared_ptr<Expression> condition,
    const hook_list& hooks, const attribute_list& attributes, const expression_list& parameters,
    const expression_list& sinks, const Location& location)
{
    if ( auto unit = ast::tryCast<type::Unit>(type) )
        return std::make_shared<type::unit::item::field::Unit>(id, unit, kind, condition, hooks,
                                                               attributes, parameters, sinks,
                                                               location);

    if ( auto list = ast::tryCast<type::List>(type) ) {
        auto field = createByType(list->argType(), nullptr, kind, nullptr, hook_list(),
                                  attribute_list(), expression_list(), expression_list(), location);
        return std::make_shared<type::unit::item::field::container::List>(id, field, kind,
                                                                          condition, hooks,
                                                                          attributes, sinks,
                                                                          location);
    }

    return std::make_shared<type::unit::item::field::AtomicType>(id, type, kind, condition, hooks,
                                                                 attributes, sinks, location);
}

unit::item::Field::Kind unit::item::Field::kind() const
{
    return _kind;
}

void unit::item::Field::setKind(Kind kind)
{
    _kind = kind;
}

bool unit::item::Field::forComposing() const
{
    return _kind == COMPOSE || _kind == PARSE_COMPOSE;
}

bool unit::item::Field::forParsing() const
{
    return _kind == PARSE || _kind == PARSE_COMPOSE;
}

bool unit::item::Field::transient() const
{
    return attributes()->has("transient");
}

/// Returns the item's associated condition, or null if none.
shared_ptr<Expression> unit::item::Field::condition() const
{
    return _cond;
}

expression_list unit::item::Field::sinks() const
{
    expression_list sinks;

    for ( auto s : _sinks )
        sinks.push_back(s);

    return sinks;
}

expression_list unit::item::Field::parameters() const
{
    expression_list params;

    for ( auto p : _params )
        params.push_back(p);

    return params;
}

unit::item::Field* unit::item::Field::parent() const
{
    return _parent;
}

void unit::item::Field::setParent(Field* parent)
{
    _parent = parent;
}

unit::item::field::Constant::Constant(shared_ptr<ID> id, shared_ptr<binpac::Constant> const_,
                                      Kind kind, shared_ptr<Expression> cond,
                                      const hook_list& hooks, const attribute_list& attrs,
                                      const expression_list& sinks, const Location& l)
    : Field(id, const_->type(), kind, cond, hooks, attrs, expression_list(), sinks, l)
{
    _const = const_;
    addChild(_const);
}

shared_ptr<binpac::Constant> unit::item::field::Constant::constant() const
{
    return _const;
}

unit::item::field::Unknown::Unknown(shared_ptr<ID> id, shared_ptr<binpac::ID> scope_id, Kind kind,
                                    shared_ptr<Expression> cond, const hook_list& hooks,
                                    const attribute_list& attrs, const expression_list& params,
                                    const expression_list& sinks, const Location& l)
    : Field(id, nullptr, kind, cond, hooks, attrs, params, sinks, l)
{
    _scope_id = scope_id;
    addChild(_scope_id);
}

shared_ptr<binpac::ID> unit::item::field::Unknown::scopeID() const
{
    return _scope_id;
}

unit::item::field::AtomicType::AtomicType(shared_ptr<ID> id, shared_ptr<binpac::Type> type,
                                          Kind kind, shared_ptr<Expression> cond,
                                          const hook_list& hooks, const attribute_list& attrs,
                                          const expression_list& sinks, const Location& l)
    : Field(id, type, kind, cond, hooks, attrs, expression_list(), sinks, l)
{
}

unit::item::field::Unit::Unit(shared_ptr<ID> id, shared_ptr<binpac::Type> type, Kind kind,
                              shared_ptr<Expression> cond, const hook_list& hooks,
                              const attribute_list& attrs, const expression_list& params,
                              const expression_list& sinks, const Location& l)
    : Field(id, type, kind, cond, hooks, attrs, params, sinks, l)
{
}

unit::item::field::Ctor::Ctor(shared_ptr<ID> id, shared_ptr<binpac::Ctor> ctor, Kind kind,
                              shared_ptr<Expression> cond, const hook_list& hooks,
                              const attribute_list& attrs, const expression_list& sinks,
                              const Location& l)
    : Field(id, ctor->type(), kind, cond, hooks, attrs, expression_list(), sinks, l)
{
    _ctor = ctor;
    addChild(_ctor);
}

shared_ptr<binpac::Ctor> unit::item::field::Ctor::ctor() const
{
    return _ctor;
}

unit::item::field::Container::Container(shared_ptr<ID> id, shared_ptr<Field> field, Kind kind,
                                        shared_ptr<Expression> cond, const hook_list& hooks,
                                        const attribute_list& attrs, const expression_list& sinks,
                                        const Location& l)
    : Field(id, std::make_shared<type::Bytes>(), kind, cond, hooks, attrs, expression_list(), sinks,
            l)
{
    _field = field;
    _field->scope()->setParent(scope());
    _field->setUnit(unit().get());

    addChild(_field);

    // Containters always get a high-priority foreach hook that adds the
    // parsed element (unless they are transient).
    if ( ! transient() ) {
        auto body_push = std::make_shared<statement::Block>(nullptr, l);

        auto self = std::make_shared<expression::ID>(std::make_shared<ID>("self", l), l);
        auto dd = std::make_shared<expression::ID>(std::make_shared<ID>("$$", l), l);
        expression_list dd_list = {dd};
        auto params = std::make_shared<constant::Tuple>(dd_list, l);
        auto name =
            std::make_shared<expression::MemberAttribute>(std::make_shared<ID>(Item::id()->name(),
                                                                               l),
                                                          l);

        expression_list ops = {self, name};
        auto op1 = std::make_shared<expression::UnresolvedOperator>(operator_::Attribute, ops, l);
        auto op2 =
            std::make_shared<expression::MemberAttribute>(std::make_shared<ID>("push_back", l), l);
        auto op3 = std::make_shared<expression::Constant>(params, l);

        ops = {op1, op2, op3};
        auto push_back =
            std::make_shared<expression::UnresolvedOperator>(operator_::MethodCall, ops, l);
        body_push->addStatement(std::make_shared<statement::Expression>(push_back, l));

        auto hook_push = std::make_shared<binpac::Hook>(body_push, binpac::Hook::PARSE, 254, false,
                                                        true, parameter_list(), l);
        addHook(hook_push);
    }

    // If they have an &until/&while, they also get another (even higher
    // priority) hook that checks the condition. For &until_including, we
    // instead do a lower priority hook.

    auto until = attributes()->lookup("until");
    auto until_including = attributes()->lookup("until_including");
    auto while_ = attributes()->lookup("while");

    if ( until || until_including || while_ ) {
        auto stop = std::make_shared<statement::Block>(nullptr, l);
        stop->addStatement(std::make_shared<statement::Stop>(l));

        auto body = std::make_shared<statement::Block>(nullptr, l);

        if ( until )
            body->addStatement(
                std::make_shared<statement::IfElse>(until->value(), stop, nullptr, l));

        if ( until_including )
            body->addStatement(
                std::make_shared<statement::IfElse>(until_including->value(), stop, nullptr, l));

        if ( while_ ) {
            expression_list ops = {while_->value()};
            auto neg = std::make_shared<expression::UnresolvedOperator>(operator_::Not, ops, l);
            body->addStatement(std::make_shared<statement::IfElse>(neg, stop, nullptr, l));
        }

        auto prio = until_including ? -255 : 255;
        auto hook = std::make_shared<binpac::Hook>(body, binpac::Hook::PARSE, prio, false, true,
                                                   parameter_list(), l);
        addHook(hook);

        if ( until )
            until->setValue(nullptr); // FIXME.

        if ( until_including )
            until_including->setValue(nullptr); // FIXME.

        if ( while_ )
            while_->setValue(nullptr); // FIXME.
    }
}

shared_ptr<unit::item::Field> unit::item::field::Container::field() const
{
    return _field;
}

unit::item::field::container::List::List(shared_ptr<ID> id, shared_ptr<Field> field, Kind kind,
                                         shared_ptr<Expression> cond, const hook_list& hooks,
                                         const attribute_list& attrs, const expression_list& sinks,
                                         const Location& l)
    : Container(id, field, kind, cond, hooks, attrs, sinks, l)
{
}

shared_ptr<binpac::Type> unit::item::field::container::List::type()
{
    // FIXME: This is odd ... We should be able to return the new type
    // directly, but if we do, some checkedTrait later fail with random
    // crashes. Not sure what's going on, but storing the type via setType()
    // appears to fix the problem.
    auto parseable = ast::type::tryTrait<type::trait::Parseable>(field()->type());
    setType(std::make_shared<type::List>(parseable->fieldType()));
    return unit::Item::type();
}

unit::item::field::container::Vector::Vector(shared_ptr<ID> id, shared_ptr<Field> field,
                                             shared_ptr<Expression> length, Kind kind,
                                             shared_ptr<Expression> cond, const hook_list& hooks,
                                             const attribute_list& attrs,
                                             const expression_list& sinks, const Location& l)
    : Container(id, field, kind, cond, hooks, attrs, sinks, l)
{
    _length = length;
    addChild(_length);
}

shared_ptr<Expression> unit::item::field::container::Vector::length() const
{
    return _length;
}


shared_ptr<binpac::Type> unit::item::field::container::Vector::type()
{
    // FIXME: This is odd ... We should be able to return the new type
    // directly, but if we do, some checkedTrait later fail with random
    // crashes. Not sure what's going on, but storing the type via setType()
    // appears to fix the problem.
    auto parseable = ast::type::tryTrait<type::trait::Parseable>(field()->type());
    setType(std::make_shared<type::Vector>(parseable->fieldType()));
    return unit::Item::type();
}

unit::item::field::switch_::Case::Case(const expression_list& exprs,
                                       shared_ptr<type::unit::item::Field> item, const Location& l)
    : Node(l)
{
    for ( auto e : exprs )
        _exprs.push_back(e);

    for ( auto e : _exprs )
        addChild(e);

    _items.push_back(item);
    addChild(_items.front());
}

unit::item::field::switch_::Case::Case(const expression_list& exprs, const unit_field_list& items,
                                       const Location& l)
    : Node(l)
{
    for ( auto e : exprs )
        _exprs.push_back(e);

    for ( auto e : _exprs )
        addChild(e);

    for ( auto i : items )
        _items.push_back(i);

    for ( auto i : _items )
        addChild(i);
}

unit::item::field::switch_::Case::Case(shared_ptr<type::unit::item::Field> item, const Location& l)
    : Node(l)
{
    _default = true;
    _items.push_back(item);
    addChild(_items.front());
}

unit::item::field::switch_::Case::Case(const unit_field_list& items, const Location& l) : Node(l)
{
    _default = true;

    for ( auto i : items )
        _items.push_back(i);

    for ( auto i : _items )
        addChild(i);
}

bool unit::item::field::switch_::Case::default_() const
{
    return _default;
}

expression_list unit::item::field::switch_::Case::expressions() const
{
    expression_list exprs;

    for ( auto e : _exprs )
        exprs.push_back(e);

    return exprs;
}

unit_field_list unit::item::field::switch_::Case::fields() const
{
    unit_field_list items;

    for ( auto i : _items )
        items.push_back(i);

    return items;
}

std::string unit::item::field::switch_::Case::uniqueName()
{
    string s;

    for ( auto f : fields() ) {
        s += (f->anonymous() ? "<anon>" : f->id()->name());
        s += "|";
        s += (f->fieldType() ? f->fieldType()->render() : "<no type>");
        s += "\n";
    }

    return ::util::uitoa_n(::util::hash(s), 62, 5);
}

unit::item::field::Switch::Switch(shared_ptr<Expression> expr, const case_list& cases, Kind kind,
                                  shared_ptr<Expression> cond, const hook_list& hooks,
                                  const Location& l)
    : Field(nullptr, nullptr, kind, cond, hooks, attribute_list(), expression_list(),
            expression_list(), l)
{
    _expr = expr;
    addChild(_expr);

    for ( auto c : cases )
        _cases.push_back(c);

    for ( auto c : _cases )
        addChild(c);

    for ( auto c : _cases ) {
        for ( auto f : c->fields() ) {
            f->scope()->setParent(scope());
            f->setParent(this);
            f->setUnit(unit().get());
        }
    }
}

shared_ptr<Expression> unit::item::field::Switch::expression() const
{
    return _expr;
}

unit::item::field::Switch::case_list unit::item::field::Switch::cases() const
{
    case_list cases;

    for ( auto c : _cases )
        cases.push_back(c);

    return cases;
}

bool unit::item::field::Switch::noFields() const
{
    for ( auto c : _cases ) {
        for ( auto f : c->fields() ) {
            if ( f->type() && ! ast::isA<type::Void>(f->type()) )
                return false;
        }
    }

    return true;
}

shared_ptr<unit::item::field::switch_::Case> unit::item::field::Switch::case_(shared_ptr<Item> f)
{
    for ( auto c : _cases ) {
        for ( auto of : c->fields() ) {
            if ( f.get() == of.get() )
                return c;
        }
    }

    return nullptr;
}

std::string unit::item::field::Switch::uniqueName()
{
    string s;

    for ( auto c : cases() ) {
        s += c->uniqueName();
        s += "===\n";
    }

    return ::util::uitoa_n(::util::hash(s), 62, 5);
}

unit::item::Variable::Variable(shared_ptr<binpac::ID> id, shared_ptr<binpac::Type> type,
                               shared_ptr<Expression> default_, const hook_list& hooks,
                               const attribute_list& attrs, const Location& l)
    : Item(id, type, hooks, attrs, l)
{
    if ( default_ )
        attributes()->add(std::make_shared<Attribute>("default", default_));
}

shared_ptr<Expression> unit::item::Variable::default_() const
{
    auto attr = attributes()->lookup("default");
    return attr ? attr->value() : nullptr;
}

unit::item::Property::Property(shared_ptr<Attribute> prop, const Location& l)
    : Item(std::make_shared<ID>("%" + prop->key()), nullptr, hook_list(), attribute_list(), l)
{
    _property = prop;
    addChild(_property);
}

shared_ptr<Attribute> unit::item::Property::Property::property() const
{
    return _property;
}

unit::item::GlobalHook::GlobalHook(shared_ptr<ID> id, hook_list& hooks, const Location& l)
    : Item(id, nullptr, hooks, attribute_list(), l)
{
}

Unit::Unit(const parameter_list& params, const unit_item_list& items, const Location& l)
{
    for ( auto p : params )
        _params.push_back(p);

    for ( auto p : _params )
        addChild(p);

    for ( auto i : items ) {
        _items.push_back(i);
        i->setUnit(this);
    }

    for ( auto d : _items )
        addChild(d);

    _scope = std::make_shared<Scope>();
}

Unit::Unit(const Location& l) : PacType(l)
{
    setWildcard(true);
}

parameter_list Unit::parameters() const
{
    parameter_list params;

    for ( auto p : _params )
        params.push_back(p);

    return params;
}

type_list Unit::parameterTypes() const
{
    type_list types;

    for ( auto p : _params )
        types.push_back(p->type());

    return types;
}

unit_item_list Unit::items() const
{
    unit_item_list items;

    for ( auto i : _items )
        items.push_back(i);

    return items;
}

static void _flatten(shared_ptr<type::unit::Item> i, unit_item_list* dst)
{
    // If it's a switch, descend.
    auto switch_ = ast::tryCast<type::unit::item::field::Switch>(i);

    if ( switch_ ) {
        for ( auto c : switch_->cases() )
            for ( auto f : c->fields() )
                _flatten(f, dst);

        // return;
    }

    dst->push_back(i);
}

unit_item_list Unit::flattenedItems() const
{
    unit_item_list items;

    for ( auto i : _items )
        _flatten(i, &items);

    return items;
}

std::list<shared_ptr<unit::item::Field>> Unit::fields() const
{
    std::list<shared_ptr<unit::item::Field>> m;

    for ( auto i : _items ) {
        auto f = ast::tryCast<unit::item::Field>(i);

        if ( f )
            m.push_back(f);
    }

    return m;
}

std::list<shared_ptr<unit::item::Field>> Unit::flattenedFields() const
{
    std::list<shared_ptr<unit::item::Field>> m;

    for ( auto i : flattenedItems() ) {
        auto f = ast::tryCast<unit::item::Field>(i);

        if ( f )
            m.push_back(f);
    }

    return m;
}

std::list<shared_ptr<unit::item::Variable>> Unit::variables() const
{
    std::list<shared_ptr<unit::item::Variable>> m;

    for ( auto i : _items ) {
        auto f = ast::tryCast<unit::item::Variable>(i);

        if ( f )
            m.push_back(f);
    }

    return m;
}

std::list<shared_ptr<unit::item::GlobalHook>> Unit::globalHooks() const
{
    std::list<shared_ptr<unit::item::GlobalHook>> m;

    for ( auto i : _items ) {
        auto f = ast::tryCast<unit::item::GlobalHook>(i);

        if ( f )
            m.push_back(f);
    }

    return m;
}

std::list<shared_ptr<unit::item::Property>> Unit::properties() const
{
    std::list<shared_ptr<unit::item::Property>> m;

    for ( auto i : _items ) {
        auto f = ast::tryCast<unit::item::Property>(i);

        if ( f )
            m.push_back(f);
    }

    return m;
}

std::list<shared_ptr<unit::item::Property>> Unit::properties(const string& name) const
{
    Attribute pp(name);

    std::list<shared_ptr<unit::item::Property>> m;

    for ( auto i : _items ) {
        auto f = ast::tryCast<unit::item::Property>(i);

        if ( ! f )
            continue;

        assert(f->property());

        if ( pp == *f->property() )
            m.push_back(i);
    }

    return m;
}

shared_ptr<unit::item::Property> Unit::property(const string& prop) const
{
    auto all = properties(prop);
    return all.size() > 0 ? all.front() : nullptr;
}

shared_ptr<unit::Item> Unit::item(shared_ptr<ID> id) const
{
    return item(id->name());
}

shared_ptr<unit::Item> Unit::item(const std::string& name) const
{
    for ( auto i : flattenedItems() ) {
        if ( i->id() && i->id()->name() == name )
            return i;
    }

    return nullptr;
}

shared_ptr<Scope> Unit::scope() const
{
    return _scope;
}

std::pair<shared_ptr<unit::Item>, string> Unit::path(string_list p, shared_ptr<Unit> current)
{
    auto next = current->item(std::make_shared<ID>(p.front()));

    if ( ! next )
        return std::make_pair(nullptr, ::util::strjoin(p, "."));

    p.pop_front();

    if ( ! p.size() )
        return std::make_pair(next, "");

    auto child = ast::tryCast<unit::item::Field>(next);

    if ( ! child )
        return std::make_pair(next, ::util::strjoin(p, "."));

    auto unit = ast::tryCast<type::Unit>(child->type());

    if ( ! unit )
        return std::make_pair(next, ::util::strjoin(p, "."));

    return path(p, unit);
}

std::pair<shared_ptr<unit::Item>, string> Unit::path(const string& p)
{
    return path(::util::strsplit(p, "."), this->sharedPtr<Unit>());
}

shared_ptr<Grammar> Unit::grammar() const
{
    return _grammar;
}

void Unit::setGrammar(shared_ptr<Grammar> grammar)
{
    _grammar = grammar;
}

bool Unit::_equal(shared_ptr<binpac::Type> ty) const
{
    auto other = ast::checkedCast<Unit>(ty);

    if ( _items.size() != other->_items.size() )
        return false;

    auto i1 = _items.begin();
    auto i2 = other->_items.begin();

    for ( ; i1 != _items.end(); ++i1, ++i2 ) {
        auto f1 = *i1;
        auto f2 = *i2;

        if ( f1->id()->name() != f2->id()->name() )
            return false;

        // Comparing the items by rendering them to avoid infinite recursion
        // for cycles.
        std::ostringstream t1;
        passes::Printer(t1, true).run(f1);

        std::ostringstream t2;
        passes::Printer(t2, true).run(f2);

        if ( t1.str() != t2.str() )
            return false;
    }

    return true;
}

bool Unit::buffering() const
{
    return _buffering;
}

void Unit::enableBuffering()
{
    _buffering = true;
}

bool Unit::trackLookAhead() const
{
    assert(_grammar);
    return _grammar->needsLookAhead();
}

bool Unit::exported() const
{
    return _exported;
}

void Unit::setExported()
{
    _exported = true;
}

bool Unit::anonymous() const
{
    return _anonymous;
}

void Unit::setAnonymous()
{
    _anonymous = true;
    auto name = util::fmt("anon_unit_%p", this);
    setID(std::make_shared<ID>(name));
}

bool Unit::ctorNoNames() const
{
    for ( auto i : _items ) {
        if ( ! i->ctorNoName() )
            return false;
    }

    return true;
}

shared_ptr<binpac::Expression> Unit::inheritedProperty(const string& pname,
                                                       shared_ptr<type::unit::Item> item)
{
    shared_ptr<binpac::Expression> expr = nullptr;

    shared_ptr<Attribute> item_property;
    shared_ptr<Attribute> unit_property;
    shared_ptr<Attribute> module_property;

    auto module = firstParent<::binpac::Module>();

    unit_property = property(pname) ? property(pname)->property() : nullptr;

    if ( module )
        module_property = module->property(pname);

    if ( item && item->attributes()->has(pname) )
        item_property = item->attributes()->lookup(pname);

    // Take most specific one.

    if ( item_property )
        return item_property->value();

    if ( unit_property )
        return unit_property->value();

    if ( module_property )
        return module_property->value();

    return nullptr;
}

bool Unit::supportsSynchronize()
{
    return property("synchronize-after") || property("synchronize-at");
}

Sink::Sink(const Location& l) : PacType(l)
{
}


Sink::~Sink()
{
}

EmbeddedObject::EmbeddedObject(shared_ptr<Type> etype, const Location& l) : TypedPacType(etype, l)
{
}

EmbeddedObject::EmbeddedObject(const Location& l) : TypedPacType(l)
{
}

Mark::Mark(const Location& l) : PacType(l)
{
}

Optional::Optional(shared_ptr<Type> type, const Location& l) : TypedPacType(type, l)
{
}

Optional::Optional(const Location& l) : TypedPacType(l)
{
}
