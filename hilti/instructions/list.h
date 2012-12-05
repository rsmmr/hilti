///
/// \type List
/// 
/// \ctor ``list(1,2,3)``, ``list()``
///
/// \cproto hlt_list*
///
/// \type List iterators.
///
/// \default List iterators initially point to the end of (any) list.
///
/// \cproto hlt_list_iter
///

#include "instructions/define-instruction.h"

iBegin(iterList, Begin, "begin")
    iTarget(optype::iterList);
    iOp1(optype::refList, true);

    iValidate {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDoc(R"(
        Returns an iterator pointing to the start of list object *op1*.
    )")
iEnd

iBegin(iterList, End, "end")
    iTarget(optype::iterList);
    iOp1(optype::refList, true);

    iValidate {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDoc(R"(
        Returns an iterator pointing to the end of list object *op1*.
    )")
iEnd

iBegin(iterList, Incr, "incr")
    iTarget(optype::iterList);
    iOp1(optype::iterList, true);

    iValidate {
        equalTypes(target, op1);
    }

    iDoc(R"(
        Advances the iterator to the next element, or to the end position if already at the end.
    )")
iEnd

iBegin(iterList, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::iterList, true);
    iOp2(optype::iterList, true);

    iValidate {
        equalTypes(op1, op2);
    }

    iDoc(R"(
        Returns True if *op1* references the same list position as *op2*.
    )")

iEnd

iBegin(iterList, Deref, "deref")
    iTarget(optype::any)
    iOp1(optype::iterList, true);

    iValidate {
        canCoerceTo(elementType(iteratedType(op1)), target);
    }

    iDoc(R"(
        Returns the element *op1* is referencing.
    )")

iEnd

iBegin(list, New, "new")
    iTarget(optype::refList)
    iOp1(optype::typeList, true)
    iOp2(optype::optional(optype::refTimerMgr), true)

    iValidate {
        equalTypes(referencedType(target), typedType(op1));
    }

    iDoc(R"(
        Instantiates a new ``list`` object of type *op1*.
    )")

iEnd


iBegin(list, Back, "list.back")
    iTarget(optype::any)
    iOp1(optype::refList, true)

    iValidate {
        canCoerceTo(elementType(referencedType(op1)), target);
    }

    iDoc(R"(    
        Returns the last element from the list referenced by *op1*. If the
        list is empty, raises an ~~Underflow exception.
    )")

iEnd

iBegin(list, Erase, "list.erase")
    iOp1(optype::iterList, false)

    iValidate {
    }

    iDoc(R"(    
        Removes the list element located by the iterator in *op1*.
    )")

iEnd

iBegin(list, Front, "list.front")
    iTarget(optype::any)
    iOp1(optype::refList, true)

    iValidate {
        canCoerceTo(elementType(referencedType(op1)), target);
    }

    iDoc(R"(    
        Returns the first element from the list referenced by *op1*. If the
        list is empty, raises an ~~Underflow exception.
    )")

iEnd

iBegin(list, Insert, "list.insert")
    iOp1(optype::any, true)
    iOp2(optype::iterList, true)

    iValidate {
        canCoerceTo(op1, elementType(iteratedType(op2)));
    }

    iDoc(R"(    
        Inserts the element *op1* into a list before the element located by
        the iterator in *op2*. If the iterator is at the end position, the
        element will become the new list tail.
    )")

iEnd

iBegin(list, PopBack, "list.pop_back")
    iTarget(optype::any)
    iOp1(optype::refList, true)

    iValidate {
        canCoerceTo(elementType(referencedType(op1)), target);
    }

    iDoc(R"(    
        Removes the last element from the list referenced by *op1* and returns
        it. If the list is empty, raises an ~~Underflow exception.
    )")

iEnd

iBegin(list, PopFront, "list.pop_front")
    iTarget(optype::any)
    iOp1(optype::refList, true)

    iValidate {
        canCoerceTo(elementType(referencedType(op1)), target);
    }

    iDoc(R"(    
        Removes the first element from the list referenced by *op1* and
        returns it. If the list is empty, raises an ~~Underflow exception.
    )")

iEnd

iBegin(list, PushBack, "list.push_back")
    iOp1(optype::refList, false)
    iOp2(optype::any, true)

    iValidate {
        canCoerceTo(op2, elementType(referencedType(op1)));
    }

    iDoc(R"(    
        Appends *op2* to the list in reference by *op1*.
    )")

iEnd

iBegin(list, PushFront, "list.push_front")
    iOp1(optype::refList, false)
    iOp2(optype::any, true)

    iValidate {
        canCoerceTo(op2, elementType(referencedType(op1)));
    }

    iDoc(R"(    
        Prepends *op2* to the list in reference by *op1*.
    )")

iEnd

iBegin(list, Append, "list.append")
    iOp1(optype::refList, false)
    iOp2(optype::refList, true)

    iValidate {
        equalTypes(elementType(referencedType(op1)), elementType(referencedType(op2)));
    }

    iDoc(R"(
        Appends all elements of *op2* to *op1*.
    )")

iEnd

iBegin(list, Size, "list.size")
    iTarget(optype::int64)
    iOp1(optype::refList, true)

    iValidate {
    }

    iDoc(R"(    
        Returns the current size of the list reference by *op1*.
    )")

iEnd

iBegin(list, Timeout, "list.timeout")
    iOp1(optype::refList, true)
    iOp2(optype::enum_, true)
    iOp3(optype::interval, true)

    iValidate {
        auto ty_op2 = as<type::Enum>(op2->type());

        // TODO: Check the enum.
    }

    iDoc(R"(    
        Activates automatic expiration of items for list *op1*. All
        subsequently inserted entries will be expired after an interval of
        *op3* after insertion (if *op2* is *Expire::Create*) or last access
        (if *op2* is *Expire::Access). Expiration is disabled if *op3* is
        zero. Throws NoTimerManager if no timer manager has been associated
        with the set at construction.
    )")

iEnd

