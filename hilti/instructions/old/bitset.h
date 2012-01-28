iBegin(bitset, Clear, "bitset.clear")
    iTarget(optype::bitset)
    iOp1(optype::bitset, trueX)
    iOp2(optype::bitset, trueX)

    iValidate {
        auto ty_target = as<type::bitset>(target->type());
        auto ty_op1 = as<type::bitset>(op1->type());
        auto ty_op2 = as<type::bitset>(op2->type());

    }

    iDoc(R"(    
        Clears the bits set in *op2* from those set in *op1* and returns the
        result. Both operands must be of the *same* ``bitset`` type.
    )")

iEnd

iBegin(bitset, Has, "bitset.has")
    iTarget(optype::bool)
    iOp1(optype::bitset, trueX)
    iOp2(optype::bitset, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::bitset>(op1->type());
        auto ty_op2 = as<type::bitset>(op2->type());

    }

    iDoc(R"(    
        Returns True if all bits in *op2* are set in *op1*. Both operands must
        be of the *same* ``bitset`` type.
    )")

iEnd

iBegin(bitset, Set, "bitset.set")
    iTarget(optype::bitset)
    iOp1(optype::bitset, trueX)
    iOp2(optype::bitset, trueX)

    iValidate {
        auto ty_target = as<type::bitset>(target->type());
        auto ty_op1 = as<type::bitset>(op1->type());
        auto ty_op2 = as<type::bitset>(op2->type());

    }

    iDoc(R"(    
        Adds the bits set in *op2* to those set in *op1* and returns the
        result. Both operands must be of the *same* ``bitset`` type.
    )")

iEnd

