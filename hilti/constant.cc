
#include <netinet/in.h>
#include <arpa/inet.h>

#include "id.h"
#include "expression.h"
#include "statement.h"
#include "module.h"
#include "constant.h"

using namespace hilti;

static shared_ptr<Type> _tuple_type(const constant::Tuple::element_list& elems, const Location& l)
{
    type::Tuple::type_list types;
    for ( auto e : elems )
        types.push_back(e->type());

    return shared_ptr<Type>(new type::Tuple(types, l));
}

constant::Tuple::Tuple(const element_list& elems, const Location& l) : Constant(l)
{
    _elems = elems;

    for ( auto e : elems )
        addChild(e);
}

shared_ptr<Type> constant::Tuple::type() const
{
    type::Tuple::type_list types;

    for ( auto e : _elems )
        types.push_back(e->type());

    return shared_ptr<type::Tuple>(new type::Tuple(types, location()));
}

shared_ptr<Type> constant::Reference::type() const
{
    return shared_ptr<type::Reference>(new type::Reference(location()));
}

constant::AddressVal::AddressVal(Constant* c, const string& addr)
{
    int result = 0;

	if ( addr.find(':') == std::string::npos ) {
        family = AddressVal::IPv4;
        result = inet_pton(AF_INET, addr.c_str(), &in.in6);
    }

	else {
        family = AddressVal::IPv6;
        result = inet_pton(AF_INET6, addr.c_str(), &in.in4);
    }

    if ( result <= 0 )
        throw ConstantParseError(c, util::fmt("cannot parse address '%s'", addr.c_str()));
}

constant::Address::Address(const string& addr, const Location& l)
    : Constant(l), _addr(this, addr)
{
}

constant::Network::Network(const string& prefix, int width, const Location& l)
    : Constant(l), _addr(this, prefix), _width(width)
{
}

constant::Network::Network(const string& cidr, const Location& l)
{
    auto i = cidr.find('/');

    if ( i == std::string::npos )
        goto error;

    _addr = AddressVal(this, cidr.substr(0, i));

    if ( util::atoi_n(cidr.begin() + i, cidr.end(), 10, &_width) != cidr.end() )
        goto error;

    return;

error:
    throw ConstantParseError(this, util::fmt("cannot parse CIDR constant '%s'", cidr.c_str()));
}

constant::Port::Port(int port, constant::PortVal::Proto proto, const Location& l) : Constant(l)
{
    _port.port = port;
    _port.proto = proto;
}

constant::Port::Port(const string& port, const Location& l)
{
    auto i = port.find('/');

    if ( i == std::string::npos )
        goto error;

    {
        auto end = util::atoi_n(port.begin(), port.begin() + i, 10, &_port.port);
        auto proto = util::strtolower(port.substr(i + 1, string::npos));

        if ( end != port.begin() + i )
            goto error;

        if ( proto == "tcp" )
            _port.proto = PortVal::TCP;

        else if ( proto == "udp" )
            _port.proto = PortVal::UDP;

        else
            goto error;
    }

    return;

error:
    throw ConstantParseError(this, util::fmt("cannot parse port constant '%s'", port.c_str()));
}

constant::Bitset::Bitset(const bit_list& bits, shared_ptr<Type> bstype, const Location& l)
{
    _bits = bits;
    _bstype = ast::as<type::Bitset>(bstype);
    assert(_bstype);

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

constant::Enum::Enum(shared_ptr<ID> value, shared_ptr<Type> etype, const Location& l) : Constant(l)
{
    _value = value;
    _etype = ast::as<type::Enum>(etype);
    assert(_etype);

    // Check that we know the label.
    for ( auto l : _etype->labels() ) {
        if ( value == l.first )
            return;
    }

    throw ConstantParseError(this, util::fmt("unknown enum label '%s'", value->pathAsString().c_str()));
}
