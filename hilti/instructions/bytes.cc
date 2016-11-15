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


iBegin(bytes::New, "new")
    iTarget(optype::refBytes);
    iOp1(optype::typeBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Instantiates a new empty bytes object.
    )");
iEnd

iBegin(bytes::Append, "bytes.append")
    iOp1(optype::refBytes, false);
    iOp2(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Appends *op2* to *op1*. The two operands must not refer to the same
        object. Raises ValueError if *op1* has been frozen, or if *op1* is the
        same as *op2*.
    )");
iEnd

iBegin(bytes::Concat, "bytes.concat")
    iTarget(optype::refBytes);
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Concatentates *op2* to *op1*, returning a new bytes object.
    )");
iEnd

iBegin(bytes::Cmp, "bytes.cmp")
    iTarget(optype::int8);
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Compares *op1* with *op2* lexicographically. If *op1* is larger, returns
        -1. If both are equal, returns 0. If *op2* is larger, returns 1.
    )");
iEnd

iBegin(bytes::Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )");
iEnd

iBegin(bytes::Copy, "bytes.copy")
    iTarget(optype::refBytes);
    iOp1(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Copy the contents of *op1* into a new byte instance.
    )");
iEnd

iBegin(bytes::Diff, "bytes.diff")
    iTarget(optype::int64);
    iOp1(optype::iterBytes, true);
    iOp2(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns the number of bytes between *op1* and *op2*.
    )");
iEnd

iBegin(bytes::Empty, "bytes.empty")
    iTarget(optype::boolean);
    iOp1(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns true if *op1* is empty. Note that using this instruction is more
        efficient than comparing the result of ``bytes.length`` to zero.
    )");
iEnd

iBegin(bytes::Freeze, "bytes.freeze")
    iOp1(optype::refBytes, false);

    iValidate
    {
    }

    iDoc(R"(
        Freezes the bytes object *op1*. A frozen bytes object cannot be
        further modified until unfrozen. If the object is already frozen, the
        instruction is ignored.
    )");
iEnd

iBegin(bytes::IsFrozenBytes, "bytes.is_frozen")
    iTarget(optype::boolean);
    iOp1(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns whether the bytes object *op1* has been frozen.
    )");
iEnd

iBegin(bytes::IsFrozenIterBytes, "bytes.is_frozen")
    iTarget(optype::boolean);
    iOp1(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns whether the bytes object referenced by the iterator *op1* has
        been frozen.
    )");
iEnd

iBegin(bytes::Length, "bytes.length")
    iTarget(optype::int64);
    iOp1(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns the number of bytes stored in *op1*.
    )");
iEnd

iBegin(bytes::Contains, "bytes.contains")
    iTarget(optype::boolean);
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns true if *op1* contains *op2* as a subsequence.
    )");
iEnd

iBegin(bytes::Find, "bytes.find")
    iTarget(optype::iterBytes);
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns an iterator representing the first occurence of *op2* in *op1*,
        or an iterator pointing to the end of *op1* if nowhere.
    )");
iEnd

iBegin(bytes::FindAtIter, "bytes.find")
    iTarget(optype::tuple);
    iOp1(optype::iterBytes, true);
    iOp2(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Searches *op2* from position *op1* onwards. Returns a tuple of a boolean
        and an iterator. If *op2* was found, the boolean will be true and the
        iterator pointing to the first occurance. If *op2* was not found, the
        boolean will be false and the iterator will point to the last position
        so that everything before that is guaranteed to not contain even a
        partial match of *op1* (in other words: one can trim until that position
        and then restart the search from there if more data gets appended to the
        underlying bytes object.
    )");
iEnd

iBegin(bytes::Offset, "bytes.offset")
    iTarget(optype::iterBytes);
    iOp1(optype::refBytes, true);
    iOp2(optype::int64, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns an iterator representing the offset *op2* in *op1*.
    )");
iEnd

iBegin(bytes::Sub, "bytes.sub")
    iTarget(optype::refBytes);
    iOp1(optype::iterBytes, true);
    iOp2(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Extracts the subsequence between *op1* and *op2* from an existing
        *bytes* instance and returns it in a new ``bytes`` instance. The element
        at *op2* is not included.
    )");
iEnd

iBegin(bytes::Trim, "bytes.trim")
    iOp1(optype::refBytes, false);
    iOp2(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Trims the bytes object *op1* at the beginning by removing data up to
        (but not including) the given position *op2*. The iterator *op2* will
        remain valid afterwards and still point to the same location, which
        will now be the first of the bytes object. Note: The effect of this
        instruction is undefined if the given iterator *op2* does not actually
        refer to a location inside the bytes object *op1*.
    )");
iEnd

iBegin(bytes::Unfreeze, "bytes.unfreeze")
    iOp1(optype::refBytes, false);

    iValidate
    {
    }

    iDoc(R"(
        Unfreezes the bytes object *op1*. An unfrozen bytes object can be
        further modified. If the object is already unfrozen (which is the
        default), the instruction is ignored.
    )");
iEnd

iBegin(bytes::ToIntFromAscii, "bytes.to_int")
    iTarget(optype::int64);
    iOp1(optype::refBytes, true);
    iOp2(optype::int64, true);

    iValidate
    {
    }

    iDoc(R"(
        Converts a bytes object *op1* into an integer, relative to given base
        *op2*.
    )");
iEnd

iBegin(bytes::ToUIntFromBinary, "bytes.to_uint")
    iTarget(optype::int64);
    iOp1(optype::refBytes, true);
    iOp2(optype::enum_, true);

    iValidate
    {
    }

    iDoc(R"(
        Converts a bytes object *op1* into an unsigned integer, assuming it's
        encoded in a binary represention with byte order *op2*.
    )");
iEnd

iBegin(bytes::ToIntFromBinary, "bytes.to_int")
    iTarget(optype::int64);
    iOp1(optype::refBytes, true);
    iOp2(optype::enum_, true);

    iValidate
    {
    }

    iDoc(R"(
        Converts a bytes object *op1* into a signed integer, assuming it's
        encoded in a binary represention with byte order *op2*.
    )");
iEnd

iBegin(bytes::Lower, "bytes.lower")
    iTarget(optype::refBytes);
    iOp1(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns a lower-case version of the bytes object *op1*.
    )");
iEnd

iBegin(bytes::Upper, "bytes.upper")
    iTarget(optype::refBytes);
    iOp1(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns an upper-case version of the bytes object *op1*.
    )");
iEnd

iBegin(bytes::Strip, "bytes.strip")
    iTarget(optype::refBytes);
    iOp1(optype::refBytes, true);
    iOp2(optype::optional(optype::enum_), true);
    iOp3(optype::optional(optype::refBytes), true);

    iValidate
    {
    }

    iDoc(R"(
        Removes leading and trailing sequences of any characters in *op2* from
        the bytes object *op1*. If *op2* is not given, removes white-space. If *op3*
        is given, it must be of type ``Hilti::Side`` and indicates which side of *op1*
        should be stripped. Default is ``Hilti::Side::Both``.
    )");
iEnd

iBegin(bytes::StartsWith, "bytes.startswith")
    iTarget(optype::boolean);
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns True if the bytes object *op1* starts with *op2*.
    )");
iEnd

iBegin(bytes::Split1, "bytes.split1")
    iTarget(optype::tuple);
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Splits bytes object *op1* at the first occurence of separator *op2*, and
        return the two parts as a tuple of bytes object. If the separator is
        not found, the returned tuple will have *op1* as its first element and
        an empty bytes object as its second. If the separator is not given, or
        empty, the split will take place a sequences of white space.
    )");
iEnd

iBegin(bytes::Split, "bytes.split")
    iTarget(optype::refVector);
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Splits bytes object *op1* at each occurence of separator *op2*, and
        return a vector with the individual pieces and the separator removes.
        If the separator is not found, the returned vector will just have *op1*
        as a single element. If the separator is not given, or empty, the split
        will take place a sequences of white space.
    )");
iEnd

iBegin(bytes::Join, "bytes.join")
    iTarget(optype::refBytes);
    iOp1(optype::refBytes, true);
    iOp2(optype::refList, true);

    iValidate
    {
    }

    iDoc(R"(
         Renders each of the elements in list *op2* into a bytes object (as
         if one printed it), and then concatenates them using *op1* as the
         separator.
    )");
iEnd

iBegin(bytes::AppendObject, "bytes.append_object")
    iOp1(optype::refBytes, false);
    iOp2(optype::any, true);

    iValidate
    {
    }

    iDoc(R"(
        Inserts a separator object *op2* to the end of the bytes object *op1*.
        When iterating over a bytes object, reaching the object will generally
        be treated as if the end has been reached. However, the instructions
        ``retrieve_object``/``at_object``/``skip_object`` may be used to operate
        on the inserted objects.
    )");
iEnd

iBegin(bytes::RetrieveObject, "bytes.retrieve_object")
    iTarget(optype::any);
    iOp1(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Retrieves a separator object at a iterator position *op1*. The object's
        type must match the target type, otherwise will throw a ``TypeError``.
        If no separator object at the position, throws a \c ValuesError.
    )");
iEnd

iBegin(bytes::AtObject, "bytes.at_object")
    iTarget(optype::boolean);
    iOp1(optype::iterBytes, true);
    iOp2(optype::optional(optype::typeAny), true);

    iValidate
    {
    }

    iDoc(R"(
        Checks if there's a separator object located at the iterator position
        *op1*. If *op2* is given, confirms that the object is of the
        corresponding type as well.
    )");
iEnd

iBegin(bytes::SkipObject, "bytes.skip_object")
    iTarget(optype::iterBytes);
    iOp1(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Advances iterator *op1* past an object, returning the new iterator.
        Throws ``ValueError`` if there's no object at the location.
    )");
iEnd

iBegin(bytes::NextObject, "bytes.next_object")
    iTarget(optype::iterBytes);
    iOp1(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Advances iterator *op1* up to the next embedded object, returning the
        new iterator or the end position of none.
    )");
iEnd

iBegin(bytes::AppendMark, "bytes.append_mark")
    iOp1(optype::refBytes, false);

    iValidate
    {
    }

    iDoc(R"(
        Inserts a mark right after the current end of bytes object *op1*. When
        later more bytes get added, the mark will be pointing to the first of
        them.
    )");
iEnd

iBegin(bytes::NextMark, "bytes.next_mark")
    iTarget(optype::iterBytes);
    iOp1(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Moves the iterator *op1* to the position of the next mark, or to then
        end if there's no further mark. Returns the current position if that's
        pointing to a mark already.
    )");
iEnd

iBegin(bytes::AtMark, "bytes.at_mark")
    iTarget(optype::boolean);
    iOp1(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns *True* if there's a mark at the iterator position *op1*.
    )");
iEnd

iBegin(bytes::Index, "bytes.index")
    iTarget(optype::int64);
    iOp1(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns offset of the byte *op1* refers to relative to the beginning of
        the underlying byte object.
    )");
iEnd

iBegin(iterBytes::Begin, "begin")
    iTarget(optype::iterBytes);
    iOp1(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns an iterator pointing to the start of bytes object *op1*.
    )");
iEnd

iBegin(iterBytes::End, "end")
    iTarget(optype::iterBytes);
    iOp1(optype::refBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns an iterator pointing to the end of bytes object *op1*.
    )");
iEnd

iBegin(iterBytes::Incr, "incr")
    iTarget(optype::iterBytes);
    iOp1(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Advances the iterator to the next element, or to the end position if
        already at the end.
    )");
iEnd

iBegin(iterBytes::IncrBy, "incr_by")
    iTarget(optype::iterBytes);
    iOp1(optype::iterBytes, true);
    iOp2(optype::int64, true);

    iValidate
    {
    }

    iDoc(R"(
        Advances the iterator by *op2* elements, or to the end position if that
        is reached during the process.
    )");
iEnd

iBegin(iterBytes::Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::iterBytes, true);
    iOp2(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns True if *op1* references the same byes position as *op2*.
    )");
iEnd

iBegin(iterBytes::Deref, "deref")
    iTarget(optype::int8);
    iOp1(optype::iterBytes, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns the bytes *op1* is referencing.
    )");
iEnd
