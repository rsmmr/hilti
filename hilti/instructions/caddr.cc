
iBegin(caddr::Function, "caddr.function")
    iTarget(optype::tuple);
    iOp1(optype::function, true);

    iValidate
    {
        auto ty_target = ast::rtti::checkedCast<type::Tuple>(target->type());

        auto cat = builder::caddr::type();
        builder::type_list tt = {cat, cat};

        equalTypes(builder::tuple::type(tt), ty_target);
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

iBegin(caddr::Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::caddr, true);
    iOp2(optype::caddr, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )")
iEnd
