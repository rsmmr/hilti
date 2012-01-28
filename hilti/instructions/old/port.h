iBegin(port, Protocol, "port.protocol")
    iTarget(optype::enum)
    iOp1(optype::port, trueX)

    iValidate {
        auto ty_target = as<type::enum>(target->type());
        auto ty_op1 = as<type::port>(op1->type());

    }

    iDoc(R"(    
        Returns the protocol of *op1*, which can currently be either
        ``Port::TCP`` or ``Port::UDP``.
    )")

iEnd

