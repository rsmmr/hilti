/// \type IP Networks
///
/// The ``net`` data type represents ranges of IP address, specified in CIDR
///   notation. It transparently handles both IPv4 and IPv6 networks.
///
/// \default ::0/0
///
/// \ctor 192.168.1.0/24, 2001:db8::1428:57ab/48
///
/// \cproto hlt_net

iBegin(network::Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::network, true);
    iOp2(optype::network, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns True if the networks in *op1* and *op2* cover the same IP
        range. Note that an IPv4 network ``a.b.c.d/n`` will match the corresponding
        IPv6 ``::a.b.c.d/n``.

        Todo: Are the v6 vs v4 matching semantics right? Should it be
        ``::ffff:a.b.c.d``? Should v4 never match a v6 address?``
    )")
iEnd

iBegin(network::EqualAddr, "equal")
    iTarget(optype::boolean);
    iOp1(optype::network, true);
    iOp2(optype::address, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns True if the addr *op2* is part of the network *op1*.
    )")
iEnd

iBegin(network::Family, "net.family")
    iTarget(optype::enum_);
    iOp1(optype::network, true);

    iValidate
    {
        auto ty_target = ast::rtti::checkedCast<type::Enum>(target->type());

        if ( ty_target->id()->pathAsString() != "Hilti::AddrFamily" )
            error(target, "target must be of type hilti::AddrFamily");
    }

    iDoc(R"(    
        Returns the address family of *op1*, which can currently be either
        ``AddrFamily::IPv4`` or ``AddrFamiliy::IPv6``.
    )")

iEnd

iBegin(network::Length, "net.length")
    iTarget(optype::int8);
    iOp1(optype::network, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns the length of the network prefix.
    )")

iEnd

iBegin(network::Prefix, "net.prefix")
    iTarget(optype::address);
    iOp1(optype::network, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns the network prefix as a masked IP addr.
    )")
iEnd

iBegin(network::Contains, "net.contains")
    iTarget(optype::boolean);
    iOp1(optype::network, true);
    iOp2(optype::address, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns true if the address *op2* is located inside the network prefix *op1*.
    )")

iEnd
