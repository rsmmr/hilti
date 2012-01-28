iBegin(end, End, "end")
    iTarget(optype::<value type>)
    iOp1(optype::ref\ <iterable>, trueX)

    iValidate {
        auto ty_target = as<type::<value type>>(target->type());
        auto ty_op1 = as<type::ref\ <iterable>>(op1->type());

    }

    iDoc(R"(    
        Returns an iterator for the end position of a sequence.
    )")

iEnd

