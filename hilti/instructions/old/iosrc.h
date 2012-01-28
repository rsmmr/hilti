iBegin(iosrc, Close, "iosrc.close")
    iOp1(optype::ref\ <iosrc>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <iosrc>>(op1->type());

    }

    iDoc(R"(    
        Closes the packet source *op1*. No further packets can then be read
        (if tried, ``pkrsrc.read`` will raise a ~~IOSrcError exception).
    )")

iEnd

iBegin(iosrc, Read, "iosrc.read")
    iTarget(optype::(time,ref\ <bytes>))
    iOp1(optype::ref\ <iosrc>, trueX)

    iValidate {
        auto ty_target = as<type::(time,ref\ <bytes>)>(target->type());
        auto ty_op1 = as<type::ref\ <iosrc>>(op1->type());

    }

    iDoc(R"(    
        Returns the next element from the I/O source *op1*. If currently no
        element is available, the instruction blocks until one is.  Raises:
        ~~IOSrcExhausted if the source has been exhausted.  Raises:
        ~~IOSrcError if there is any other problem with returning the next
        element.
    )")

iEnd

