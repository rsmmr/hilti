
iBegin(caddr, Function, "caddr.function")
    iTarget(optype::tuple)
    iOp1(optype::function, true)

    iValidate {
        //auto ty_target = as<type::(caddr,caddr)>(target->type());
        //auto ty_op1 = as<type::function>(op1->type());
        //
        // TODO: Make sure we have an (caddr, caddr) tuple.

    }

    iDoc(R"(    
        Returns the physical address of a function. The function must be of
        linkage ~~EXPORT, and the instruction returns two separate addresses.
        For functions of calling convention ~~HILTI, the first is that of the
        compiled function itself and the second is that of the corresponding
        ~~resume function. For functions of calling convention ~~C and
        ~~C_HILTI, only the first element of the target tuple is used, and the
        second is undefined. *op1* must be a constant.
    )")

iEnd

