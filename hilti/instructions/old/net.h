iBegin(net, Family, "net.family")
    iTarget(optype::enum)
    iOp1(optype::net, trueX)

    iValidate {
        auto ty_target = as<type::enum>(target->type());
        auto ty_op1 = as<type::net>(op1->type());

    }

    iDoc(R"(    
        Returns the address family of *op1*, which can currently be either
        ``AddrFamily::IPv4`` or ``AddrFamiliy::IPv6``.
    )")

iEnd

iBegin(net, Length, "net.length")
    iTarget(optype::int\ <8>)
    iOp1(optype::net, trueX)

    iValidate {
        auto ty_target = as<type::int\ <8>>(target->type());
        auto ty_op1 = as<type::net>(op1->type());

    }

    iDoc(R"(    
        Returns the length of the network prefix.
    )")

iEnd

iBegin(net, Prefix, "net.prefix")
    iTarget(optype::addr)
    iOp1(optype::net, trueX)

    iValidate {
        auto ty_target = as<type::addr>(target->type());
        auto ty_op1 = as<type::net>(op1->type());

    }

    iDoc(R"(    
        Returns the network prefix as a masked IP addr.
    )")

iEnd

