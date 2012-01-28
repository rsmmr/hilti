iBegin(hook, DisableGroup, "hook.disable_group")
    iOp1(optype::int\ <64>, trueX)

    iValidate {
        auto ty_op1 = as<type::int\ <64>>(op1->type());

    }

    iDoc(R"(    
        Disables the hook group given by *op1* globally.
    )")

iEnd

iBegin(hook, EnableGroup, "hook.enable_group")
    iOp1(optype::int\ <64>, trueX)

    iValidate {
        auto ty_op1 = as<type::int\ <64>>(op1->type());

    }

    iDoc(R"(    
        Enables the hook group given by *op1* globally.
    )")

iEnd

iBegin(hook, GroupEnabled, "hook.group_enabled")
    iTarget(optype::bool)
    iOp1(optype::int\ <64>, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::int\ <64>>(op1->type());

    }

    iDoc(R"(    
        Sets *target* to ``True`` if hook group *op1* is enabled, and to
        *False* otherwise.
    )")

iEnd

iBegin(hook, Run, "hook.run")
    iTarget(optype::[type])
    iOp1(optype::Hook, trueX)
    iOp2(optype::tuple, trueX)

    iValidate {
        auto ty_target = as<type::[type]>(target->type());
        auto ty_op1 = as<type::Hook>(op1->type());
        auto ty_op2 = as<type::tuple>(op2->type());

    }

    iDoc(R"(    
        Executes the hook *op1* with arguments *op2*, storing the hook's
        return value in *target*.
    )")

iEnd

iBegin(hook, Stop, "hook.stop")
    iOp1(optype::[type], trueX)

    iValidate {
        auto ty_op1 = as<type::[type]>(op1->type());

    }

    iDoc(R"(    
        Stops the execution of the current hook and returns *op1* as the hooks
        value (if one is needed).
    )")

iEnd

