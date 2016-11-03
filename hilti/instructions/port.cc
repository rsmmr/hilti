/// \type Ports
///
/// The *port* data type represents TCP and UDP port numbers.
///
/// \default 0/tcp
///
/// \ctor 80/tcp, 53/udp
///
/// \cproto hlt_port


iBegin(port::Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::port, true);
    iOp2(optype::port, true);

    iValidate {
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )");
iEnd

iBegin(port::Protocol, "port.protocol")
    iTarget(optype::enum_);
    iOp1(optype::port, true);

    iValidate {
        auto ty_target = ast::rtti::checkedCast<type::Enum>(target->type());

        if ( util::strtolower(ty_target->id()->pathAsString()) != "hilti::protocol" )
            error(target, "target must be of type hilti::Protocol");
    }

    iDoc(R"(
        Returns the protocol of *op1*, which can currently be either
        ``Port::TCP`` or ``Port::UDP``.
    )");
iEnd
