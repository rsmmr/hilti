iBegin(addr, Family, "addr.family")
    iTarget(optype::enum)
    iOp1(optype::addr, trueX)

    iValidate {
        auto ty_target = as<type::enum>(target->type());
        auto ty_op1 = as<type::addr>(op1->type());

    }

    iDoc(R"(    
        Returns the address family of *op1*, which can be either
        :hlt:glob:`hilti::AddrFamily::IPv4` or
        :hlt:glob:`hilti::AddrFamily::IPv6`.
    )")

iEnd

