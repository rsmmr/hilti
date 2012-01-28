iBegin(struct, Get, "struct.get")
    iTarget(optype::any)
    iOp1(optype::ref\ <struct>, trueX)
    iOp2(optype::string, trueX)

    iValidate {
        auto ty_target = as<type::any>(target->type());
        auto ty_op1 = as<type::ref\ <struct>>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());

    }

    iDoc(R"(    
        Returns the field named *op2* in the struct referenced by *op1*. The
        field name must be a constant, and the type of the target must match
        the field's type. If a field is requested that has not been set, its
        default value is returned if it has any defined. If it has not, an
        ``UndefinedValue`` exception is raised.
    )")

iEnd

iBegin(struct, Get, "struct.get_default")
    iTarget(optype::any)
    iOp1(optype::ref\ <struct>, trueX)
    iOp2(optype::string, trueX)
    iOp3(optype::any, trueX)

    iValidate {
        auto ty_target = as<type::any>(target->type());
        auto ty_op1 = as<type::ref\ <struct>>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());
        auto ty_op3 = as<type::any>(op3->type());

    }

    iDoc(R"(    
        Returns the field named *op2* in the struct referenced by *op1*, or a
        default value *op3* if not set (if the field has a default itself,
        that however has priority). The field name must be a constant, and the
        type of the target must match the field's type, as must that of the
        default value.
    )")

iEnd

iBegin(struct, IsSet, "struct.is_set")
    iTarget(optype::bool)
    iOp1(optype::ref\ <struct>, trueX)
    iOp2(optype::string, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::ref\ <struct>>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());

    }

    iDoc(R"(    
        Returns *True* if the field named *op2* has been set to a value, and
        *False otherwise. If the instruction returns *True*, a subsequent call
        to ~~Get will not raise an exception.
    )")

iEnd

iBegin(struct, Set, "struct.set")
    iOp1(optype::ref\ <struct>, trueX)
    iOp2(optype::string, trueX)
    iOp3(optype::any, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <struct>>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());
        auto ty_op3 = as<type::any>(op3->type());

    }

    iDoc(R"(    
        Sets the field named *op2* in the struct referenced by *op1* to the
        value *op3*. The type of the *op3* must match the type of the field.
    )")

iEnd

iBegin(struct, Unset, "struct.unset")
    iOp1(optype::ref\ <struct>, trueX)
    iOp2(optype::string, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <struct>>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());

    }

    iDoc(R"(    
        Unsets the field named *op2* in the struct referenced by *op1*. An
        unset field appears just as if it had never been assigned an value; in
        particular, it will be reset to its default value if has been one
        assigned.
    )")

iEnd

