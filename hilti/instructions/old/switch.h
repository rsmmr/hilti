iBegin(switch, Switch, "switch")
    iOp1(optype::<value type>, trueX)
    iOp2(optype::Label, trueX)
    iOp3(optype::( (value, destination), ...), trueX)

    iValidate {
        auto ty_op1 = as<type::<value type>>(op1->type());
        auto ty_op2 = as<type::Label>(op2->type());
        auto ty_op3 = as<type::( (value, destination), ...)>(op3->type());

    }

    iDoc(R"(    
        Branches to one of several alternatives. *op1* determines which
        alternative to take.  *op3* is a tuple of giving all alternatives as
        2-tuples *(value, destination)*. *value* must be of the same type as
        *op1*, and *destination* is a block label. If *value* equals *op1*,
        control is transfered to the corresponding block. If multiple
        alternatives match *op1*, one of them is taken but it's undefined
        which one. If no alternative matches, control is transfered to block
        *op2*.
    )")

iEnd

