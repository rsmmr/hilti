iBegin(channel, Read, "channel.read")
    iTarget(optype::*item-type*)
    iOp1(optype::ref\ <channel>, trueX)

    iValidate {
        auto ty_target = as<type::*item-type*>(target->type());
        auto ty_op1 = as<type::ref\ <channel>>(op1->type());

    }

    iDoc(R"(    
        Returns the next channel item from the channel referenced by *op1*. If
        the channel is empty, the instruction blocks until an item becomes
        available.
    )")

iEnd

iBegin(channel, ReadTry, "channel.read_try")
    iTarget(optype::*item-type*)
    iOp1(optype::ref\ <channel>, trueX)

    iValidate {
        auto ty_target = as<type::*item-type*>(target->type());
        auto ty_op1 = as<type::ref\ <channel>>(op1->type());

    }

    iDoc(R"(    
        Returns the next channel item from the channel referenced by *op1*. If
        the channel is empty, the instruction raises a ``WouldBlock``
        exception.
    )")

iEnd

iBegin(channel, Size, "channel.size")
    iTarget(optype::int\ <64>)
    iOp1(optype::ref\ <channel>, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::ref\ <channel>>(op1->type());

    }

    iDoc(R"(    
        Returns the current number of items in the channel referenced by
        *op1*.
    )")

iEnd

iBegin(channel, Write, "channel.write")
    iOp1(optype::ref\ <channel>, trueX)
    iOp2(optype::*item-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <channel>>(op1->type());
        auto ty_op2 = as<type::*item-type*>(op2->type());

    }

    iDoc(R"(    
        Writes an item into the channel referenced by *op1*. If the channel is
        full, the instruction blocks until a slot becomes available.
    )")

iEnd

iBegin(channel, WriteTry, "channel.write_try")
    iOp1(optype::ref\ <channel>, trueX)
    iOp2(optype::*item-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <channel>>(op1->type());
        auto ty_op2 = as<type::*item-type*>(op2->type());

    }

    iDoc(R"(    
        Writes an item into the channel referenced by *op1*. If the channel is
        full, the instruction raises a ``WouldBlock`` exception.
    )")

iEnd

