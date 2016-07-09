
#include "define-instruction.h"

#include "../module.h"
#include "bytes.h"

iBeginCC(bytes)
    iValidateCC(New)
    {
    }

    iDocCC(New, R"(
        Instantiates a new empty bytes object.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Append)
    {
    }

    iDocCC(Append, R"(
        Appends *op2* to *op1*. The two operands must not refer to the same
        object.  Raises ValueError if *op1* has been frozen, or if *op1* is
        the same as *op2*.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Concat)
    {
    }

    iDocCC(Concat, R"(
        Concatentates *op2* to *op1*, returning a new bytes object.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Cmp)
    {
    }

    iDocCC(Cmp, R"(
        Compares *op1* with *op2* lexicographically. If *op1* is larger,
        returns -1. If both are equal, returns 0. If *op2* is larger, returns
        1.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Equal)
    {
    }

    iDocCC(Equal, R"(
        Returns true if *op1* is equal to *op2*.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Copy)
    {
    }

    iDocCC(Copy, R"(
        Copy the contents of *op1* into a new byte instance.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(Diff)
    {
    }

    iDocCC(Diff, R"(
        Returns the number of bytes between *op1* and *op2*.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Empty)
    {
    }

    iDocCC(Empty, R"(
        Returns true if *op1* is empty. Note that using this instruction is
        more efficient than comparing the result of ``bytes.length`` to zero.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Freeze)
    {
    }

    iDocCC(Freeze, R"(
        Freezes the bytes object *op1*. A frozen bytes object cannot be
        further modified until unfrozen. If the object is already frozen, the
        instruction is ignored.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(IsFrozenBytes)
    {
    }

    iDocCC(IsFrozenBytes, R"(
        Returns whether the bytes object *op1* has been frozen.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(IsFrozenIterBytes)
    {
    }

    iDocCC(IsFrozenIterBytes, R"(
        Returns whether the bytes object referenced by the iterator *op1* has been frozen.
    )")

iEndCC


iBeginCC(bytes)
    iValidateCC(Length)
    {
    }

    iDocCC(Length, R"(
        Returns the number of bytes stored in *op1*.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Contains)
    {
    }

    iDocCC(Contains, R"(
        Returns true if *op1* contains *op2* as a subsequence.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Find)
    {
    }

    iDocCC(Find, R"(
        Returns an iterator representing the first occurence of *op2* in *op1*, or an iterator pointing to the end of *op1* if nowhere.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(FindAtIter)
    {
        // TODO: Validate type of target tuple.
    }

    iDocCC(FindAtIter, R"(
        Searches *op2* from position *op1* onwards. Returns a tuple of a boolean and an iterator.
        If *op2* was found, the boolean will be true and the iterator pointing to the first occurance.
        If *op2* was not found, the boolean will be false and the iterator will point to the last position
        so that everything before that is guaranteed to not contain even a partial match of *op1* (in other
        words: one can trim until that position and then restart the search from there if more data gets
        appended to the underlying bytes object.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Offset)
    {
    }

    iDocCC(Offset, R"(
        Returns an iterator representing the offset *op2* in *op1*.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Sub)
    {
    }

    iDocCC(Sub, R"(
        Extracts the subsequence between *op1* and *op2* from an existing
        *bytes* instance and returns it in a new ``bytes`` instance. The
        element at *op2* is not included.
    )")

iEndCC

iBeginCC(bytes)
    iValidateCC(Trim)
    {
    }

    iDocCC(Trim, R"(
        Trims the bytes object *op1* at the beginning by removing data up to
        (but not including) the given position *op2*. The iterator *op2* will
        remain valid afterwards and still point to the same location, which
        will now be the first of the bytes object.  Note: The effect of this
        instruction is undefined if the given iterator *op2* does not actually
        refer to a location inside the bytes object *op1*.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(Unfreeze)
    {
    }

    iDocCC(Unfreeze, R"(
        Unfreezes the bytes object *op1*. An unfrozen bytes object can be
        further modified. If the object is already unfrozen (which is the
        default), the instruction is ignored.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(ToIntFromAscii)
    {
    }

    iDocCC(ToIntFromAscii, R"(
        Converts a bytes object *op1* into an integer, relative to given base *op2*.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(ToUIntFromBinary)
    {
    }

    iDocCC(ToUIntFromBinary, R"(
        Converts a bytes object *op1* into an unsigned integer, assuming it's encoded in a binary represention with byte order *op2*.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(ToIntFromBinary)
    {
    }

    iDocCC(ToIntFromBinary, R"(
        Converts a bytes object *op1* into a signed integer, assuming it's encoded in a binary represention with byte order *op2*.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(Lower)
    {
    }

    iDocCC(Lower, R"(
        Returns a lower-case version of the bytes object *op1*.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(Upper)
    {
    }

    iDocCC(Upper, R"(
        Returns an upper-case version of the bytes object *op1*.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(Strip)
    {
    }

    iDocCC(Strip, R"(
        Removes leading and trailing sequences of any characters in *op2* from the bytes object *op1*. If *op2* is not given, removes white-space. If *op3* is given, it must be of type ``Hilti::Side`` and indicates which side of *op1* should be stripped. Default is ``Hilti::Side::Both``.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(StartsWith)
    {
    }

    iDocCC(StartsWith, R"(
        Returns True if the bytes object *op1* starts with *op2*.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(Split1)
    {
        // TODO: Check tuple type.
    }

    iDocCC(Split1, R"(
        Splits bytes object *op1* at the first occurence of separator *op2*, and return the two parts as a tuple of bytes object. If the separator is not found, the returned tuple will have *op1* as its first element and an empty bytes object as its second. If the separator is not given, or empty, the split will take place a sequences of white space.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(Split)
    {
        // TODO: Check vector type.
    }

    iDocCC(Split, R"(
        Splits bytes object *op1* at each occurence of separator *op2*, and return a vector with the individual pieces and the separator removes. If the separator is not found, the returned vector will just have *op1* as a single element.  If the separator is not given, or empty, the split will take place a sequences of white space.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(Join)
    {
    }

    iDocCC(Join, R"(
         Renders each of the elements in list *op2* into a bytes object (as if one printed it), and then concatenates them using *op1* as the separator.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(AppendObject)
    {
    }

    iDocCC(
        AppendObject,
        R"(Inserts a separator object *op2* to the end of the bytes object *op1*. When iterating over a bytes object, reaching the object will generally be treated as if the end has been reached. However, the  instructions ``retrieve_object``/``at_object``/``skip_object`` may be used to operate on the inserted objects.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(RetrieveObject)
    {
    }

    iDocCC(
        RetrieveObject,
        R"(Retrieves a separator object at a iterator position *op1*. The object's type must match the target type, otherwise will throw a ``TypeError``. If no separator object at the position, throws a \c ValuesError.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(AtObject)
    {
    }

    iDocCC(
        AtObject,
        R"(Checks if there's a separator object located at the iterator position *op1*. If *op2* is given, confirms that the object is of the corresponding type as well.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(SkipObject)
    {
    }

    iDocCC(
        SkipObject,
        R"(Advances iterator *op1* past an object, returning the new iterator. Throws ``ValueError`` if there's no object at the location.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(NextObject)
    {
    }

    iDocCC(
        NextObject,
        R"(Advances iterator *op1* up to the next embedded object, returning the new iterator or the end position of none.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(AppendMark)
    {
    }

    iDocCC(
        AppendMark,
        R"(Inserts a mark right after the current end of bytes object *op1*. When later more bytes get added, the mark will be pointing to the first of them.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(NextMark)
    {
    }

    iDocCC(
        NextMark,
        R"(Moves the iterator *op1* to the position of the next mark, or to then end if there's no further mark. Returns the current position if that's pointing to a mark already.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(AtMark)
    {
    }

    iDocCC(AtMark, R"(Checks if there's a mark located at the iterator position *op1*.
    )")
iEndCC

iBeginCC(bytes)
    iValidateCC(Index)
    {
    }

    iDocCC(Index, R"(
        Returns offset of the byte *op1* refers to relative to the beginning of the underlying byte object.
    )")

iEndCC

iBeginCC(iterBytes)
    iValidateCC(Begin)
    {
    }

    iDocCC(Begin, R"(
        Returns an iterator pointing to the start of bytes object *op1*.
    )")
iEndCC

iBeginCC(iterBytes)
    iValidateCC(End)
    {
    }

    iDocCC(End, R"(
        Returns an iterator pointing to the end of bytes object *op1*.
    )")
iEndCC

iBeginCC(iterBytes)
    iValidateCC(Incr)
    {
    }

    iDocCC(Incr, R"(
        Advances the iterator to the next element, or to the end position if already at the end.
    )")
iEndCC

iBeginCC(iterBytes)
    iValidateCC(IncrBy)
    {
    }

    iDocCC(IncrBy, R"(
        Advances the iterator by *op2* elements, or to the end position
        if that is reached during the process.
    )")
iEndCC

iBeginCC(iterBytes)
    iValidateCC(Equal)
    {
    }

    iDocCC(Equal, R"(
        Returns True if *op1* references the same byes position as *op2*.
    )")

iEndCC

iBeginCC(iterBytes)
    iValidateCC(Deref)
    {
    }

    iDocCC(Deref, R"(
        Returns the bytes *op1* is referencing.
    )")
iEndCC
