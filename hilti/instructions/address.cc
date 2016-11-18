/// \type IP Addresses
///
/// The ``addr`` data type represents IP addresses, and it transparently
/// handles both IPv4 and IPv6 addresses.
///
/// \default ::0
///
/// \ctor 192.168.1.1, 2001:db8::1428:57ab
///
/// \cproto hlt_addr

iBegin(address::Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::address, true);
    iOp2(optype::address, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )");

iEnd

iBegin(address::Family, "addr.family")
    iTarget(optype::enum_);
    iOp1(optype::address, true);

    iValidate
    {
        auto ty_target = ast::rtti::checkedCast<type::Enum>(target->type());

        if ( ty_target->id()->pathAsString() != "Hilti::AddrFamily" )
            error(target, "target must be of type Hilti::AddrFamily");
    }

    iDoc(R"(    
        Returns the address family of *op1*, which can
        be either :hlt:glob:`Hilti::AddrFamily::IPv4` or
        :hlt:glob:`Hilti::AddrFamily::IPv6`.
    )");

iEnd
