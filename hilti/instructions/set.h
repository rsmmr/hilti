///
/// \type Set
///
/// A ``set`` store keys of a particular type ``K``. Insertions, lookups, and
/// deletes are amortized constant time. Keys must be HILTI *value types*, and
/// each value can only be stored once.
///
/// Sets are iterable, yet the order in which elements are iterated over
/// is undefined.
///
/// Todo: Add note on semantics when modifying the hash table while iterating over
/// it.
///
/// .. todo:: When resizig, load spikes can occur for large set. We should extend
/// the hash table implementation to do resizes incrementally.
///
/// \ctor ``set(1,2,3)``, ``set()``
///
/// \cproto hlt_set*
///
/// \type ``iter<set<*>>``
///
/// \default Set iterators initially point to the end of (any) list.
///
/// \cproto hlt_set_iter
///

#include "define-instruction.h"

iBegin(iterSet, Begin, "begin")
    iTarget(optype::iterSet);
    iOp1(optype::refSet, true);

    iValidate
    {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDoc(R"(
        Returns an iterator pointing to the start of set object *op1*.
    )")
iEnd

iBegin(iterSet, End, "end")
    iTarget(optype::iterSet);
    iOp1(optype::refSet, true);

    iValidate
    {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDoc(R"(
        Returns an iterator pointing to the end of set object *op1*.
    )")
iEnd

iBegin(iterSet, Incr, "incr")
    iTarget(optype::iterSet);
    iOp1(optype::iterSet, true);

    iValidate
    {
        equalTypes(target, op1);
    }

    iDoc(R"(
        Advances the iterator to the next element, or to the end position if already at the end.
    )")
iEnd

iBegin(iterSet, Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::iterSet, true);
    iOp2(optype::iterSet, true);

    iValidate
    {
        equalTypes(op1, op2);
    }

    iDoc(R"(
        Returns True if *op1* references the same set position as *op2*.
    )")

iEnd

iBegin(iterSet, Deref, "deref")
    iTarget(optype::any);
    iOp1(optype::iterSet, true);

    iValidate
    {
        canCoerceTo(elementType(iteratedType(op1)), target);
    }

    iDoc(R"(
        Returns the set *op1* is referencing.
    )")

iEnd


iBegin(set, New, "new")
    iTarget(optype::refSet);
    iOp1(optype::typeSet, true);
    iOp2(optype::optional(optype::refTimerMgr), false);

    iValidate
    {
        equalTypes(referencedType(target), typedType(op1));
    }

    iDoc(R"(
        Instantiates a new ``set`` object of type *op1*.
    )")

iEnd


iBegin(set, Clear, "set.clear")
    iOp1(optype::refSet, false);

    iValidate
    {
    }

    iDoc(R"(
        Removes all entries from set *op1*.
    )")

iEnd

iBegin(set, Exists, "set.exists")
    iTarget(optype::boolean);
    iOp1(optype::refSet, true);
    iOp2(optype::any, true);

    iValidate
    {
        canCoerceTo(op2, elementType(referencedType(op1)));
    }

    iDoc(R"(
        Checks whether the key *op2* exists in set *op1*. If so, the
        instruction returns True, and False otherwise.
    )")

iEnd

iBegin(set, Insert, "set.insert")
    iOp1(optype::refSet, false);
    iOp2(optype::any, false);

    iValidate
    {
        canCoerceTo(op2, elementType(referencedType(op1)));
    }

    iDoc(R"(
        Sets the element at index *op2* in set *op1* to *op3. If the key
        already exists, the previous entry is replaced.
    )")

iEnd

iBegin(set, Remove, "set.remove")
    iOp1(optype::refSet, false);
    iOp2(optype::any, true);

    iValidate
    {
        canCoerceTo(op2, elementType(referencedType(op1)));
    }

    iDoc(R"(
        Removes the key *op2* from the set *op1*. If the key does not exists,
        the instruction has no effect.
    )")

iEnd

iBegin(set, Size, "set.size")
    iTarget(optype::int64);
    iOp1(optype::refSet, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns the current number of entries in set *op1*.
    )")

iEnd

iBegin(set, Timeout, "set.timeout")
    iOp1(optype::refSet, true);
    iOp2(optype::enum_, true);
    iOp3(optype::interval, true);

    iValidate
    {
        auto ty_op2 = ast::rtti::checkedCast<type::Enum>(op2->type());

        // TODO: Check the enum.
    }

    iDoc(R"(
        Activates automatic expiration of items for set *op1*. All
        subsequently inserted entries will be expired after an interval of
        *op3* after they (if *op2* is *Expire::Create*) or last accessed (if
        *op2* is *Expire::Access). Expiration is disable if *op3* is zero.
        Throws NoTimerManager if no timer manager has been associated with the
        set at construction.
    )")

iEnd
