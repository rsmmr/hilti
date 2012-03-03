///
/// \type Bytes
///
/// *bytes* is a data type for storing sequences of raw bytes. It is
/// optimized for storing and operating on large amounts of unstructured
/// data. In particular, it provides efficient subsequence and append
/// operations. Bytes are forward-iterable.
///
/// \default *bytes* instances default to being empty.
///
/// \ctor The constructor for ``bytes`` resembles the one for strings:
/// ``b"abcdef"`` creates a new ``bytes`` object and initialized it with six
/// bytes. Note that such ``bytes`` instances are *not* constants but can be
/// modified later on.
///
/// \cproto hlt_bytes*
///
/////
///
/// \type ``iter<bytes>``
///
/// \default A ``bytes`` iterator is initially set to the end of (any)
/// ``bytes`` object.
///
/// \cproto hlt_bytes_iter
///

iBegin(bytes, New, "new")
    iTarget(optype::refBytes)
    iOp1(optype::typeBytes, true);

    iValidate {
    }

    iDoc(R"(
        Instantiates a new empty bytes object.
    )")

iEnd

iBegin(bytes, Append, "bytes.append")
    iOp1(optype::refBytes, false);
    iOp2(optype::refBytes, true);

    iValidate {
    }

    iDoc(R"(
        Appends *op2* to *op1*. The two operands must not refer to the same
        object.  Raises ValueError if *op1* has been frozen, or if *op1* is
        the same as *op2*.
    )")

iEnd

iBegin(bytes, Cmp, "bytes.cmp")
    iTarget(optype::int8)
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);

    iValidate {
    }

    iDoc(R"(
        Compares *op1* with *op2* lexicographically. If *op1* is larger,
        returns -1. If both are equal, returns 0. If *op2* is larger, returns
        1.
    )")

iEnd

iBegin(bytes, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);

    iValidate {
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )")

iEnd

iBegin(bytes, Copy, "bytes.copy")
    iTarget(optype::refBytes)
    iOp1(optype::refBytes, true);

    iValidate {
        auto ty_target = as<type::Bytes>(target->type());
        auto ty_op1 = as<type::Bytes>(op1->type());

    }

    iDoc(R"(
        Copy the contents of *op1* into a new byte instance.
    )")

iEnd

iBegin(bytes, Diff, "bytes.diff")
    iTarget(optype::int64)
    iOp1(optype::iterBytes, true);
    iOp2(optype::iterBytes, true);

    iValidate {
    }

    iDoc(R"(
        Returns the number of bytes between *op1* and *op2*.
    )")

iEnd

iBegin(bytes, Empty, "bytes.empty")
    iTarget(optype::boolean)
    iOp1(optype::refBytes, true);

    iValidate {
    }

    iDoc(R"(
        Returns true if *op1* is empty. Note that using this instruction is
        more efficient than comparing the result of ``bytes.length`` to zero.
    )")

iEnd

iBegin(bytes, Freeze, "bytes.freeze")
    iOp1(optype::refBytes, false);

    iValidate {
    }

    iDoc(R"(
        Freezes the bytes object *op1*. A frozen bytes object cannot be
        further modified until unfrozen. If the object is already frozen, the
        instruction is ignored.
    )")

iEnd

iBegin(bytes, IsFrozenBytes, "bytes.is_frozen")
    iTarget(optype::boolean)
    iOp1(optype::refBytes, true);

    iValidate {
    }

    iDoc(R"(
        Returns whether the bytes object *op1* has been frozen.
    )")

iEnd

iBegin(bytes, IsFrozenIterBytes, "bytes.is_frozen")
    iTarget(optype::boolean)
    iOp1(optype::iterBytes, true);

    iValidate {
    }

    iDoc(R"(
        Returns whether the bytes object referenced by the iterator *op1* has been frozen.
    )")

iEnd


iBegin(bytes, Length, "bytes.length")
    iTarget(optype::int64)
    iOp1(optype::refBytes, true);

    iValidate {
    }

    iDoc(R"(
        Returns the number of bytes stored in *op1*.
    )")

iEnd

iBegin(bytes, Offset, "bytes.offset")
    iTarget(optype::iterBytes)
    iOp1(optype::refBytes, true);
    iOp2(optype::int64, true);

    iValidate {
    }

    iDoc(R"(
        Returns an iterator representing the offset *op2* in *op1*.
    )")

iEnd

iBegin(bytes, Sub, "bytes.sub")
    iTarget(optype::refBytes)
    iOp1(optype::iterBytes, true);
    iOp2(optype::iterBytes, true);

    iValidate {
    }

    iDoc(R"(
        Extracts the subsequence between *op1* and *op2* from an existing
        *bytes* instance and returns it in a new ``bytes`` instance. The
        element at *op2* is not included.
    )")

iEnd

iBegin(bytes, Trim, "bytes.trim")
    iOp1(optype::refBytes, false);
    iOp2(optype::iterBytes, true);

    iValidate {

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
    iOp1(optype::refBytes, false);

    iValidate {

    }

    iDoc(R"(
        Unfreezes the bytes object *op1*. An unfrozen bytes object can be
        further modified. If the object is already unfrozen (which is the
        default), the instruction is ignored.
    )")

iEnd

iBegin(iterBytes, Begin, "begin")
    iTarget(optype::iterBytes);
    iOp1(optype::refBytes, true);

    iValidate {
    }

    iDoc(R"(
        Returns an iterator pointing to the start of bytes object *op1*.
    )")
iEnd

iBegin(iterBytes, End, "end")
    iTarget(optype::iterBytes);
    iOp1(optype::refBytes, true);

    iValidate {
    }

    iDoc(R"(
        Returns an iterator pointing to the end of bytes object *op1*.
    )")
iEnd

iBegin(iterBytes, Incr, "incr")
    iTarget(optype::iterBytes);
    iOp1(optype::iterBytes, true);

    iValidate {
    }

    iDoc(R"(
        Advances the iterator to the next element, or to the end position if already at the end.
    )")
iEnd

iBegin(iterBytes, IncrBy, "incr_by")
    iTarget(optype::iterBytes);
    iOp1(optype::iterBytes, true);
    iOp2(optype::int64, true);

    iValidate {
    }

    iDoc(R"(
        Advances the iterator by *op2* elements, or to the end position
        if that is reached during the process.
    )")
iEnd

iBegin(iterBytes, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::iterBytes, true);
    iOp2(optype::iterBytes, true);

    iValidate {
    }

    iDoc(R"(
        Returns True if *op1* references the same byes position as *op2*.
    )")

iEnd

iBegin(iterBytes, Deref, "deref")
    iTarget(optype::int8)
    iOp1(optype::iterBytes, true);

    iValidate {
    }

    iDoc(R"(
        Returns the bytes *op1* is referencing.
    )")

iEnd

