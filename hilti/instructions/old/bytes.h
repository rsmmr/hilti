iBegin(bytes, Append, "bytes.append")
    iOp1(optype::ref\ <bytes>, trueX)
    iOp2(optype::ref\ <bytes>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <bytes>>(op1->type());
        auto ty_op2 = as<type::ref\ <bytes>>(op2->type());

    }

    iDoc(R"(    
        Appends *op2* to *op1*. The two operands must not refer to the same
        object.  Raises ValueError if *op1* has been frozen, or if *op1* is
        the same as *op2*.
    )")

iEnd

iBegin(bytes, Cmp, "bytes.cmp")
    iTarget(optype::int\ <8>)
    iOp1(optype::ref\ <bytes>, trueX)
    iOp2(optype::ref\ <bytes>, trueX)

    iValidate {
        auto ty_target = as<type::int\ <8>>(target->type());
        auto ty_op1 = as<type::ref\ <bytes>>(op1->type());
        auto ty_op2 = as<type::ref\ <bytes>>(op2->type());

    }

    iDoc(R"(    
        Compares *op1* with *op2* lexicographically. If *op1* is larger,
        returns -1. If both are equal, returns 0. If *op2* is larger, returns
        1.
    )")

iEnd

iBegin(bytes, Copy, "bytes.copy")
    iTarget(optype::ref\ <bytes>)
    iOp1(optype::ref\ <bytes>, trueX)

    iValidate {
        auto ty_target = as<type::ref\ <bytes>>(target->type());
        auto ty_op1 = as<type::ref\ <bytes>>(op1->type());

    }

    iDoc(R"(    
        Copy the contents of *op1* into a new byte instance.
    )")

iEnd

iBegin(bytes, Diff, "bytes.diff")
    iTarget(optype::int\ <64>)
    iOp1(optype::iterator<bytes>, trueX)
    iOp2(optype::iterator<bytes>, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::iterator<bytes>>(op1->type());
        auto ty_op2 = as<type::iterator<bytes>>(op2->type());

    }

    iDoc(R"(    
        Returns the number of bytes between *op1* and *op2*.
    )")

iEnd

iBegin(bytes, Empty, "bytes.empty")
    iTarget(optype::bool)
    iOp1(optype::ref\ <bytes>, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::ref\ <bytes>>(op1->type());

    }

    iDoc(R"(    
        Returns true if *op1* is empty. Note that using this instruction is
        more efficient than comparing the result of ``bytes.length`` to zero.
    )")

iEnd

iBegin(bytes, Freeze, "bytes.freeze")
    iOp1(optype::ref\ <bytes>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <bytes>>(op1->type());

    }

    iDoc(R"(    
        Freezes the bytes object *op1*. A frozen bytes object cannot be
        further modified until unfrozen. If the object is already frozen, the
        instruction is ignored.
    )")

iEnd

iBegin(bytes, IsFrozen, "bytes.is_frozen")
    iTarget(optype::bool)
    iOp1(optype::ref<bytes> or iterator<bytes>, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::ref<bytes> or iterator<bytes>>(op1->type());

    }

    iDoc(R"(    
        Returns whether the bytes object *op1* (or the bytes objects referred
        to be the iterator *op1*) has been frozen.
    )")

iEnd

iBegin(bytes, Length, "bytes.length")
    iTarget(optype::int\ <64>)
    iOp1(optype::ref\ <bytes>, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::ref\ <bytes>>(op1->type());

    }

    iDoc(R"(    
        Returns the number of bytes stored in *op1*.
    )")

iEnd

iBegin(bytes, Offset, "bytes.offset")
    iTarget(optype::iterator<bytes>)
    iOp1(optype::ref\ <bytes>, trueX)
    iOp2(optype::int\ <64>, trueX)

    iValidate {
        auto ty_target = as<type::iterator<bytes>>(target->type());
        auto ty_op1 = as<type::ref\ <bytes>>(op1->type());
        auto ty_op2 = as<type::int\ <64>>(op2->type());

    }

    iDoc(R"(    
        Returns an iterator representing the offset *op2* in *op1*.
    )")

iEnd

iBegin(bytes, Sub, "bytes.sub")
    iTarget(optype::ref\ <bytes>)
    iOp1(optype::iterator<bytes>, trueX)
    iOp2(optype::iterator<bytes>, trueX)

    iValidate {
        auto ty_target = as<type::ref\ <bytes>>(target->type());
        auto ty_op1 = as<type::iterator<bytes>>(op1->type());
        auto ty_op2 = as<type::iterator<bytes>>(op2->type());

    }

    iDoc(R"(    
        Extracts the subsequence between *op1* and *op2* from an existing
        *bytes* instance and returns it in a new ``bytes`` instance. The
        element at *op2* is not included.
    )")

iEnd

iBegin(bytes, Trim, "bytes.trim")
    iOp1(optype::ref\ <bytes>, trueX)
    iOp2(optype::iterator<bytes>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <bytes>>(op1->type());
        auto ty_op2 = as<type::iterator<bytes>>(op2->type());

    }

    iDoc(R"(    
        Trims the bytes object *op1* at the beginning by removing data up to
        (but not including) the given position *op2*. The iterator *op2* will
        remain valid afterwards and still point to the same location, which
        will now be the first of the bytes object.  Note: The effect of this
        instruction is undefined if the given iterator *op2* does not actually
        refer to a location inside the bytes object *op1*.
    )")

iEnd

iBegin(bytes, Unfreeze, "bytes.unfreeze")
    iOp1(optype::ref\ <bytes>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <bytes>>(op1->type());

    }

    iDoc(R"(    
        Unfreezes the bytes object *op1*. An unfrozen bytes object can be
        further modified. If the object is already unfrozen (which is the
        default), the instruction is ignored.
    )")

iEnd

