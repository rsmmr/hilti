
iBegin(operator_, Assign, "assign")
    iTarget(optype::any)
    iOp1(optype::any, true)

    iValidate {
        auto ty_target = as<type::ValueType>(target->type());
        auto ty_op1 = as<type::ValueType>(op1->type());

        if ( ! ty_target ) {
            error(target, "target must be a value type");
            return;
        }

        if ( ! ty_op1 ) {
            error(op1, "operand must be a value type");
            return;
        }

        if ( op1->canCoerceTo(ty_target) )
            error(op1, "operand not compatible with target");
    }

    iDoc(R"(    
        Assigns *op1* to the target.  There is a short-cut syntax: instead of
        using the standard form ``t = assign op``, one can just write ``t =
        op``.
    )")
iEnd
