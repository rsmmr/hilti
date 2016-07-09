///
/// \type Vector
///
/// A ``vector`` maps integer indices to elements of a specific type ``T``.
/// Vector elements can be read and written. Indices are zero-based. For a
/// read operation, all indices smaller or equal the largest index written so
/// far are valid. If an index is read that has not been yet written to, type
/// T's default value is returned. Read operations are fast, as are
/// insertions as long as the indices of inserted elements are not larger
/// than the vector's last index. Inserting can be expensive however if an
/// index beyond the vector's current end is given.
///
/// Vectors are forward-iterable. An iterator always corresponds to a
/// specific index and it is therefore safe to modify the vector even while
/// iterating over it. After any change, the iterator will locate the element
/// which is now located at the index it is pointing at. Once an iterator has
/// reached the end-position however, it will never return an element anymore
/// (but raise an ~~InvalidIterator exception), even if more are added to the
/// vector.
///
/// \ctor ``vector(1,2,3)``, ``vector()``
///
/// \cproto hlt_vector*
///
/// \type ``iter<vector<*>>``
///
/// \default Vector iterators initially point to the end of (any) vector.
///
/// \cproto hlt_vector_iter
///

#include "define-instruction.h"

iBegin(iterVector, Begin, "begin")
    iTarget(optype::iterVector);
    iOp1(optype::refVector, true);

    iValidate
    {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDoc(R"(
        Returns an iterator pointing to the start of vector object *op1*.
    )")
iEnd

iBegin(iterVector, End, "end")
    iTarget(optype::iterVector);
    iOp1(optype::refVector, true);

    iValidate
    {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDoc(R"(
        Returns an iterator pointing to the end of vector object *op1*.
    )")
iEnd

iBegin(iterVector, Incr, "incr")
    iTarget(optype::iterVector);
    iOp1(optype::iterVector, true);

    iValidate
    {
        equalTypes(target, op1);
    }

    iDoc(R"(
        Advances the iterator to the next element, or to the end position if already at the end.
    )")
iEnd

iBegin(iterVector, Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::iterVector, true);
    iOp2(optype::iterVector, true);

    iValidate
    {
        equalTypes(op1, op2);
    }

    iDoc(R"(
        Returns True if *op1* references the same vector position as *op2*.
    )")

iEnd

iBegin(iterVector, Deref, "deref")
    iTarget(optype::any);
    iOp1(optype::iterVector, true);

    iValidate
    {
        canCoerceTo(elementType(iteratedType(op1)), target);
    }

    iDoc(R"(
        Returns the vector *op1* is referencing.
    )")

iEnd

iBegin(vector, New, "new")
    iTarget(optype::refVector);
    iOp1(optype::typeVector, true);
    iOp2(optype::optional(optype::refTimerMgr), false);

    iValidate
    {
        equalTypes(referencedType(target), typedType(op1));
    }

    iDoc(R"(
        Instantiates a new ``vector`` object of type *op1*.
    )")

iEnd

iBegin(vector, Get, "vector.get")
    iTarget(optype::any);
    iOp1(optype::refVector, true);
    iOp2(optype::int64, true);

    iValidate
    {
        canCoerceTo(elementType(referencedType(op1)), target);
    }

    iDoc(R"(    
        Returns the element at index *op2* in vector *op1*.
    )")

iEnd

iBegin(vector, Exists, "vector.exists")
    iTarget(optype::boolean);
    iOp1(optype::refVector, true);
    iOp2(optype::int64, true);

    iValidate
    {
    }

    iDoc(R"(
        Checks whether the index *op2* has been assinged a value in *op1*.
    )")

iEnd

iBegin(vector, PushBack, "vector.push_back")
    iOp1(optype::refVector, false);
    iOp2(optype::any, false);

    iValidate
    {
        canCoerceTo(op2, elementType(referencedType(op1)));
    }

    iDoc(R"(    
        Sets the element at index *op2* in vector *op1* to *op3.
    )")

iEnd

iBegin(vector, Reserve, "vector.reserve")
    iOp1(optype::refVector, false);
    iOp2(optype::int64, true);

    iValidate
    {
    }

    iDoc(R"(    
        Reserves space for at least *op2* elements in vector *op1*. This
        operations does not change the vector in any observable way but rather
        gives a hint to the implementation about the size that will be needed.
        The implemenation may use this information to avoid unnecessary memory
        allocations.
    )")

iEnd

iBegin(vector, Set, "vector.set")
    iOp1(optype::refVector, false);
    iOp2(optype::int64, true);
    iOp3(optype::any, false);

    iValidate
    {
        canCoerceTo(op3, elementType(referencedType(op1)));
    }

    iDoc(R"(    
        Sets the element at index *op2* in vector *op1* to *op3.
    )")

iEnd

iBegin(vector, Size, "vector.size")
    iTarget(optype::int64);
    iOp1(optype::refVector, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns the current size of the vector *op1*, which is the largest
        accessible index plus one.
    )")

iEnd

iBegin(vector, Timeout, "vector.timeout")
    iOp1(optype::refVector, true);
    iOp2(optype::enum_, true);
    iOp3(optype::interval, true);

    iValidate
    {
        auto ty_op2 = ast::rtti::checkedCast<type::Enum>(op2->type());

        // TODO: Check enum.
    }

    iDoc(R"(    
        Activates automatic expiration of items for vector *op1*. All
        subsequently inserted entries will be expired after an interval of
        *op3* after insertion (if *op2* is *Expire::Create*) or last access
        (if *op2* is *Expire::Access). Expired entries are set back to
        uninitialized. Expiration is disabled if *op3* is zero. Throws
        NoTimerManager if no timer manager has been associated with the set at
        construction.
    )")

iEnd
