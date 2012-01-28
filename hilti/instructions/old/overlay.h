iBegin(overlay, Attach, "overlay.attach")
    iOp1(optype::overlay, trueX)
    iOp2(optype::iterator<bytes>, trueX)

    iValidate {
        auto ty_op1 = as<type::overlay>(op1->type());
        auto ty_op2 = as<type::iterator<bytes>>(op2->type());

    }

    iDoc(R"(    
        Associates the overlay in *op1* with a bytes object, aligning the
        overlays offset 0 with the position specified by *op2*. The *attach*
        operation must be performed before any other ``overlay`` instruction
        is used on *op1*.
    )")

iEnd

iBegin(overlay, Get, "overlay.get")
    iTarget(optype::any)
    iOp1(optype::overlay, trueX)
    iOp2(optype::string, trueX)

    iValidate {
        auto ty_target = as<type::any>(target->type());
        auto ty_op1 = as<type::overlay>(op1->type());
        auto ty_op2 = as<type::string>(op2->type());

    }

    iDoc(R"(    
        Unpacks the field named *op2* from the bytes object attached to the
        overlay *op1*. The field name must be a constant, and the type of the
        target must match the field's type.  The instruction throws an
        OverlayNotAttached exception if ``overlay.attach`` has not been
        executed for *op1* yet.
    )")

iEnd

