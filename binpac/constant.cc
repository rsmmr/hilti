
#include <netinet/in.h>
#include <arpa/inet.h>

#include "constant.h"
#include "id.h"
#include "expression.h"
#include "type.h"

using namespace binpac;
using namespace binpac::constant;

Constant::Constant(const Location& l)
    : ast::Constant<AstInfo>(l)
{
}

ConstantParseError::ConstantParseError(Constant* c, const string& msg)
    : ast::RuntimeError(msg, c)
{
}


ConstantParseError::ConstantParseError(shared_ptr<Constant> c, const string& msg)
    : ast::RuntimeError(msg, c.get())
{
}

constant::Expression::Expression(shared_ptr<Type> type, shared_ptr<binpac::Expression> expr, const Location& l)
{
    _expr = expr;
    _type = type ? type : expr->type();

    addChild(_expr);
    addChild(_type);
}

shared_ptr<binpac::Expression> constant::Expression::expression() const
{
    return _expr;
}

shared_ptr<Type> constant::Expression::type() const
{
    return _type;
}

String::String(const string& value, const Location& l)
    : ast::SpecificConstant<AstInfo, Constant, string>(value, shared_ptr<Type>(new type::String()), l)
{
}

Integer::Integer(int64_t value, int width, bool sign, const Location& l)
    : ast::SpecificConstant<AstInfo, Constant, int64_t>(value, shared_ptr<Type>(new type::Integer(width, sign)), l)
{
}

Bool::Bool(bool value, const Location& l)
    : ast::SpecificConstant<AstInfo, Constant, bool>(value, shared_ptr<Type>(new type::Bool()), l)
{
}

Address::Address(const string& addr, const Location& l)
    : ast::SpecificConstant<AstInfo, Constant, string>(addr, shared_ptr<Type>(new type::String()), l)
{
}

Network::Network(const string& net, const Location& l)
    : ast::SpecificConstant<AstInfo, Constant, string>(net, shared_ptr<Type>(new type::String()), l)
{
}

Network::Network(const string& net, int width, const Location& l)
    : ast::SpecificConstant<AstInfo, Constant, string>(::util::fmt("%s/%d", net.c_str(), width), shared_ptr<Type>(new type::String()), l)
{
}

Port::Port(const string& port, const Location& l)
    : ast::SpecificConstant<AstInfo, Constant, string>(port, shared_ptr<Type>(new type::String()), l)
{
}

Double::Double(double value, const Location& l)
    : ast::SpecificConstant<AstInfo, Constant, double>(value, shared_ptr<Type>(new type::Double()), l)
{
}

Time::Time(double time, const Location& l) : Constant(l)
{
    _time = (uint64_t) (time * 1e9);
}

Time::Time(uint64_t time, const Location& l) : Constant(l)
{
    _time = (uint64_t) (time * 1e9);
}

shared_ptr<Type> Time::type() const {
    return shared_ptr<Type>(new type::Time());
}

Interval::Interval(uint64_t interv, const Location& l) : Constant(l)
{
    _interv = (uint64_t) (interv * 1e9);
}

Interval::Interval(double interv, const Location& l) : Constant(l)
{
    _interv = (uint64_t) (interv * 1e9);
}

shared_ptr<Type> Interval::type() const
{
    return shared_ptr<Type>(new type::Interval());
}

static shared_ptr<Type> _tuple_type(const expression_list& elems, const Location& l)
{
    type::Tuple::type_list types;
    for ( auto e : elems )
        types.push_back(e->type());

    return shared_ptr<Type>(new type::Tuple(types, l));
}

Bitset::Bitset(const bit_list& bits, shared_ptr<Type> bstype, const Location& l)
{
    _bits = bits;
    _bstype = ast::checkedCast<type::Bitset>(bstype);

    // Check that we know all the labels.
    for ( auto b : bits ) {
        bool found = false;

        for ( auto l : _bstype->labels() ) {
            if ( b == l.first ) {
                found = 1;
                break;
            }
        }

        if ( ! found )
            throw ConstantParseError(this, util::fmt("unknown enum label '%s'", b->pathAsString().c_str()));
    }
}

shared_ptr<Type> Bitset::type() const
{
    return _bstype;
}

const Bitset::bit_list& Bitset::value() const
{
    return _bits;
}

Enum::Enum(shared_ptr<ID> value, shared_ptr<Type> etype, const Location& l) : Constant(l)
{
    _value = value;
    _etype = ast::checkedCast<type::Enum>(etype);

    // Check that we know the label.
    for ( auto l : _etype->labels() ) {
        if ( value == l.first )
            return;
    }

    throw ConstantParseError(this, util::fmt("unknown enum label '%s'", value->pathAsString().c_str()));
}

shared_ptr<Type> Enum::type() const
{
    return _etype;
}

shared_ptr<ID> Enum::value() const
{
    return _value;
}

Tuple::Tuple(const expression_list& elems, const Location& l) : Constant(l)
{
    for ( auto e : elems )
        _elems.push_back(e);

    for ( auto e : _elems )
        addChild(e);
}

shared_ptr<Type> Tuple::type() const
{
    type::Tuple::type_list types;

    for ( auto e : _elems )
        types.push_back(e->type());

    return shared_ptr<type::Tuple>(new type::Tuple(types, location()));
}

expression_list Tuple::value() const
{
    expression_list l;

    for ( auto e : _elems )
        l.push_back(e);

    return l;
}

uint64_t Time::value() const
{
    return _time;
}

const uint64_t Interval::value() const
{
    return _interv;
}
