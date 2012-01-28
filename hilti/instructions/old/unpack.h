iBegin(unpack, Unpack, "unpack")
    iTarget(optype::(type,iterator<bytes>))
    iOp1(optype::(iterator<bytes>,iterator<bytes>), trueX)
    iOp2(optype::enum, trueX)
    iOp3(optype::[type], trueX)

    iValidate {
        auto ty_target = as<type::(type,iterator<bytes>)>(target->type());
        auto ty_op1 = as<type::(iterator<bytes>,iterator<bytes>)>(op1->type());
        auto ty_op2 = as<type::enum>(op2->type());
        auto ty_op3 = as<type::[type]>(op3->type());

    }

    iDoc(R"(    
        Unpacks an instance of a particular type (as determined by *target*;
        see below) from the binary data enclosed by the iterator tuple *op1*.
        *op2* defines the binary layout as an enum of type ``Hilti::Packed``
        and must be a constant. Depending on *op2*, *op3* is may be an
        additional, format-specific parameter with further information about
        the binary layout. The operator returns a ``tuple<T,
        iterator<bytes>>``, in the first component is the newly unpacked
        instance and the second component is locates the first bytes that has
        *not* been consumed anymore.  Raises ~~WouldBlock if there are not
        sufficient bytes available for unpacking the type. Can also raise
        other exceptions if other errors occur, in particular if the raw bytes
        are not as expected (and that fact can be verified).  Note: The
        ``unpack`` operator uses a generic implementation able to handle all
        data types. Different from most other operators, it's implementation
        is not overloaded on a per-type based. However, each type must come
        with an ~~unpack decorated function, which the generic operator
        implementatin relies on for doing the unpacking.  Note: We should
        define error semantics more crisply. Ideally, a single UnpackError
        should be raised in all error cases.
    )")

iEnd

