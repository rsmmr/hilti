
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

        if ( ! op1->canCoerceTo(ty_target) )
            error(op1, "operand not compatible with target");
    }

    iDoc(R"(    
        Assigns *op1* to the target.  There is a short-cut syntax: instead of
        using the standard form ``t = assign op``, one can just write ``t =
        op``.
    )")
iEnd

iBegin(operator_, Unpack, "unpack")
    iTarget(optype::tuple)
    iOp1(optype::tuple, true)
    iOp2(optype::tuple, true)
    iOp3(optype::optional(optype::any), true)

    iValidate {
        // TODO
    }

    iDoc(R"(
    Unpacks an instance of a particular type (as determined by *target*;
    see below) from the binary data enclosed by the iterator tuple *op1*.
    *op2* defines the binary layout as an enum of type ``Hilti::Packed`` and
    must be a constant. Depending on *op2*, *op3* is may be an additional,
    format-specific parameter with further information about the binary
    layout. The operator returns a ``tuple<T, iterator<bytes>>``, in the first
    component is the newly unpacked instance and the second component is
    locates the first bytes that has *not* been consumed anymore.

    Raises ~~WouldBlock if there are not sufficient bytes available
    for unpacking the type. Can also raise ``UnpackError` if the raw bytes
    are not as expected (and that fact can be verified).

    Note: The ``unpack`` operator uses a generic implementation able to handle all data
    types. Different from most other operators, it's implementation is not
    overloaded on a per-type based. Instead, the type specific code is implemented in the codegen::Unpacker.
    )")
iEnd
