iBegin(map, Clear, "map.clear")
    iOp1(optype::ref\ <map>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <map>>(op1->type());

    }

    iDoc(R"(    
        Removes all entries from map *op1*.
    )")

iEnd

iBegin(map, Default, "map.default")
    iOp1(optype::ref\ <map>, trueX)
    iOp2(optype::*value-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <map>>(op1->type());
        auto ty_op2 = as<type::*value-type*>(op2->type());

    }

    iDoc(R"(    
        Sets a default value *op2* for map *op1* to be returned by *map.get*
        if a key does not exist.
    )")

iEnd

iBegin(map, Exists, "map.exists")
    iTarget(optype::bool)
    iOp1(optype::ref\ <map>, trueX)
    iOp2(optype::*key-type*, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::ref\ <map>>(op1->type());
        auto ty_op2 = as<type::*key-type*>(op2->type());

    }

    iDoc(R"(    
        Checks whether the key *op2* exists in map *op1*. If so, the
        instruction returns True, and False otherwise.
    )")

iEnd

iBegin(map, Get, "map.get")
    iTarget(optype::*value-type*)
    iOp1(optype::ref\ <map>, trueX)
    iOp2(optype::*key-type*, trueX)

    iValidate {
        auto ty_target = as<type::*value-type*>(target->type());
        auto ty_op1 = as<type::ref\ <map>>(op1->type());
        auto ty_op2 = as<type::*key-type*>(op2->type());

    }

    iDoc(R"(    
        Returns the element with key *op2* in map *op1*. Throws IndexError if
        the key does not exists and no default has been set via *map.default*.
    )")

iEnd

iBegin(map, GetDefault, "map.get_default")
    iTarget(optype::*value-type*)
    iOp1(optype::ref\ <map>, trueX)
    iOp2(optype::*key-type*, trueX)
    iOp3(optype::*value-type*, trueX)

    iValidate {
        auto ty_target = as<type::*value-type*>(target->type());
        auto ty_op1 = as<type::ref\ <map>>(op1->type());
        auto ty_op2 = as<type::*key-type*>(op2->type());
        auto ty_op3 = as<type::*value-type*>(op3->type());

    }

    iDoc(R"(    
        Returns the element with key *op2* in map *op1* if it exists. If the
        key does not exists, returns *op3*.
    )")

iEnd

iBegin(map, Insert, "map.insert")
    iOp1(optype::ref\ <map>, trueX)
    iOp2(optype::*key-type*, trueX)
    iOp3(optype::*value-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <map>>(op1->type());
        auto ty_op2 = as<type::*key-type*>(op2->type());
        auto ty_op3 = as<type::*value-type*>(op3->type());

    }

    iDoc(R"(    
        Sets the element at index *op2* in map *op1* to *op3. If the key
        already exists, the previous entry is replaced.
    )")

iEnd

iBegin(map, Remove, "map.remove")
    iOp1(optype::ref\ <map>, trueX)
    iOp2(optype::*key-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <map>>(op1->type());
        auto ty_op2 = as<type::*key-type*>(op2->type());

    }

    iDoc(R"(    
        Removes the key *op2* from the map *op1*. If the key does not exists,
        the instruction has no effect.
    )")

iEnd

iBegin(map, Size, "map.size")
    iTarget(optype::int\ <64>)
    iOp1(optype::ref\ <map>, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::ref\ <map>>(op1->type());

    }

    iDoc(R"(    
        Returns the current number of entries in map *op1*.
    )")

iEnd

iBegin(map, Timeout, "map.timeout")
    iOp1(optype::ref\ <map>, trueX)
    iOp2(optype::enum, trueX)
    iOp3(optype::interval, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <map>>(op1->type());
        auto ty_op2 = as<type::enum>(op2->type());
        auto ty_op3 = as<type::interval>(op3->type());

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

