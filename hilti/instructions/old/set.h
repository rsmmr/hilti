iBegin(set, Clear, "set.clear")
    iOp1(optype::ref\ <set>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <set>>(op1->type());

    }

    iDoc(R"(    
        Removes all entries from set *op1*.
    )")

iEnd

iBegin(set, Exists, "set.exists")
    iTarget(optype::bool)
    iOp1(optype::ref\ <set>, trueX)
    iOp2(optype::*key-type*, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::ref\ <set>>(op1->type());
        auto ty_op2 = as<type::*key-type*>(op2->type());

    }

    iDoc(R"(    
        Checks whether the key *op2* exists in set *op1*. If so, the
        instruction returns True, and False otherwise.
    )")

iEnd

iBegin(set, Insert, "set.insert")
    iOp1(optype::ref\ <set>, trueX)
    iOp2(optype::*key-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <set>>(op1->type());
        auto ty_op2 = as<type::*key-type*>(op2->type());

    }

    iDoc(R"(    
        Sets the element at index *op2* in set *op1* to *op3. If the key
        already exists, the previous entry is replaced.
    )")

iEnd

iBegin(set, Remove, "set.remove")
    iOp1(optype::ref\ <set>, trueX)
    iOp2(optype::*key-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <set>>(op1->type());
        auto ty_op2 = as<type::*key-type*>(op2->type());

    }

    iDoc(R"(    
        Removes the key *op2* from the set *op1*. If the key does not exists,
        the instruction has no effect.
    )")

iEnd

iBegin(set, Size, "set.size")
    iTarget(optype::int\ <64>)
    iOp1(optype::ref\ <set>, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::ref\ <set>>(op1->type());

    }

    iDoc(R"(    
        Returns the current number of entries in set *op1*.
    )")

iEnd

iBegin(set, Timeout, "set.timeout")
    iOp1(optype::ref\ <set>, trueX)
    iOp2(optype::enum, trueX)
    iOp3(optype::interval, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <set>>(op1->type());
        auto ty_op2 = as<type::enum>(op2->type());
        auto ty_op3 = as<type::interval>(op3->type());

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

