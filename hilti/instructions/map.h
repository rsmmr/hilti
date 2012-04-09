///
/// \type Maps
///
/// A ``maps`` maps keys of type ``K`` to values of type ``T``. Insertions,
/// lookups, and deletes are amortized constant time. Keys must be HILTI *value
/// types*, while values can be of any time. Up to one value can be associated
/// with each key.
/// 
/// Maps are iterable, yet the order in which elements are iterated over is
/// undefined.
/// 
/// .. todo::Add note on semantics when modifying the hash table while iterating
/// over it.
/// 
/// .. todo:: When resizig, load spikes can occur for large tables. We should
/// extend the hash table implementation to do resizes incrementally.
/// 
/// \ctor map(1: "hurz", map(2: "test")), map()
///
/// \cproto hlt_map*
///
/// \type ``iter<map<*>>``
///
/// \default Map iterators initially point to the end of (any) list.
///
/// \cproto hlt_map_iter
///

#include "instructions/define-instruction.h"

iBegin(iterMap, Begin, "begin")
    iTarget(optype::iterMap);
    iOp1(optype::refMap, true);

    iValidate {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDoc(R"(
        Returns an iterator pointing to the start of map object *op1*.
    )")
iEnd

iBegin(iterMap, End, "end")
    iTarget(optype::iterMap);
    iOp1(optype::refMap, true);

    iValidate {
        equalTypes(iteratedType(target), referencedType(op1));
    }

    iDoc(R"(
        Returns an iterator pointing to the end of map object *op1*.
    )")
iEnd

iBegin(iterMap, Incr, "incr")
    iTarget(optype::iterMap);
    iOp1(optype::iterMap, true);

    iValidate {
        equalTypes(target, op1);
    }

    iDoc(R"(
        Advances the iterator to the next element, or to the end position if already at the end.
    )")
iEnd

iBegin(iterMap, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::iterMap, true);
    iOp2(optype::iterMap, true);

    iValidate {
        equalTypes(target, op1);
    }

    iDoc(R"(
        Returns True if *op1* references the same map position as *op2*.
    )")

iEnd

iBegin(iterMap, Deref, "deref")
    iTarget(optype::any)
    iOp1(optype::iterMap, true);

    iValidate {
        canCoerceTo(mapValueType(iteratedType(op1)), target);
    }

    iDoc(R"(
        Returns the map *op1* is referencing.
    )")

iEnd


iBegin(map, New, "new")
    iTarget(optype::refMap)
    iOp1(optype::typeMap, true)
    iOp2(optype::optional(optype::refTimerMgr), true)

    iValidate {
        hasType(target, typedType(op1));
    }

    iDoc(R"(
        Instantiates a new ``map`` object of type *op1*.
    )")

iEnd


iBegin(map, Clear, "map.clear")
    iOp1(optype::refMap, false)

    iValidate {
    }

    iDoc(R"(    
        Removes all entries from map *op1*.
    )")

iEnd

iBegin(map, Default, "map.default")
    iOp1(optype::refMap, false)
    iOp2(optype::any, true)

    iValidate {
        canCoerceTo(op2, mapValueType(op1));
    }

    iDoc(R"(    
        Sets a default value *op2* for map *op1* to be returned by *map.get*
        if a key does not exist.
    )")

iEnd

iBegin(map, Exists, "map.exists")
    iTarget(optype::boolean)
    iOp1(optype::refMap, true)
    iOp2(optype::any, true)

    iValidate {
        canCoerceTo(op2, mapValueType(op1));
    }

    iDoc(R"(    
        Checks whether the key *op2* exists in map *op1*. If so, the
        instruction returns True, and False otherwise.
    )")

iEnd

iBegin(map, Get, "map.get")
    iTarget(optype::any)
    iOp1(optype::refMap, true)
    iOp2(optype::any, true)

    iValidate {
        canCoerceTo(op2, mapKeyType(op1));
        canCoerceTo(mapValueType(op1), target);
    }

    iDoc(R"(    
        Returns the element with key *op2* in map *op1*. Throws IndexError if
        the key does not exists and no default has been set via *map.default*.
    )")

iEnd

iBegin(map, GetDefault, "map.get_default")
    iTarget(optype::any)
    iOp1(optype::refMap, true)
    iOp2(optype::any, true)
    iOp3(optype::any, true)

    iValidate {
        canCoerceTo(op2, mapKeyType(op1));
        canCoerceTo(op3, mapValueType(op1));
        canCoerceTo(mapValueType(op1), target);
    }

    iDoc(R"(    
        Returns the element with key *op2* in map *op1* if it exists. If the
        key does not exists, returns *op3*.
    )")

iEnd

iBegin(map, Insert, "map.insert")
    iOp1(optype::refMap, false)
    iOp2(optype::any, true)
    iOp3(optype::any, true)

    iValidate {
        canCoerceTo(op2, mapKeyType(op1));
        canCoerceTo(op3, mapValueType(op1));
    }

    iDoc(R"(    
        Sets the element at index *op2* in map *op1* to *op3. If the key
        already exists, the previous entry is replaced.
    )")

iEnd

iBegin(map, Remove, "map.remove")
    iOp1(optype::refMap, false)
    iOp2(optype::any, true)

    iValidate {
        canCoerceTo(op2, mapKeyType(op1));
    }

    iDoc(R"(    
        Removes the key *op2* from the map *op1*. If the key does not exists,
        the instruction has no effect.
    )")

iEnd

iBegin(map, Size, "map.size")
    iTarget(optype::int64)
    iOp1(optype::refMap, true)

    iValidate {
    }

    iDoc(R"(    
        Returns the current number of entries in map *op1*.
    )")

iEnd

iBegin(map, Timeout, "map.timeout")
    iOp1(optype::refMap, true)
    iOp2(optype::enum_, true)
    iOp3(optype::interval, true)

    iValidate {
        auto ty_op2 = as<type::Enum>(op2->type());

        // TODO: Check enum.

    }

    iDoc(R"(    
        Activates automatic expiration of items for map *op1*. All
        subsequently inserted entries will be expired after an interval of
        *op3* after they have been added (if *op2* is *Expire::Create*) or
        last accessed (if *op2* is *Expire::Access). Expiration is disable if
        *op3* is zero. Throws NoTimerManager if no timer manager has been
        associated with the map at construction.
    )")

iEnd

