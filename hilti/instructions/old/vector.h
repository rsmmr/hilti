iBegin(vector, Get, "vector.get")
    iTarget(optype::*item-type*)
    iOp1(optype::ref\ <vector>, trueX)
    iOp2(optype::int\ <64>, trueX)

    iValidate {
        auto ty_target = as<type::*item-type*>(target->type());
        auto ty_op1 = as<type::ref\ <vector>>(op1->type());
        auto ty_op2 = as<type::int\ <64>>(op2->type());

    }

    iDoc(R"(    
        Returns the element at index *op2* in vector *op1*.
    )")

iEnd

iBegin(vector, Set, "vector.push_back")
    iOp1(optype::ref\ <vector>, trueX)
    iOp2(optype::*item-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <vector>>(op1->type());
        auto ty_op2 = as<type::*item-type*>(op2->type());

    }

    iDoc(R"(    
        Sets the element at index *op2* in vector *op1* to *op3.
    )")

iEnd

iBegin(vector, Reserve, "vector.reserve")
    iOp1(optype::ref\ <vector>, trueX)
    iOp2(optype::int\ <64>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <vector>>(op1->type());
        auto ty_op2 = as<type::int\ <64>>(op2->type());

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
    iOp1(optype::ref\ <vector>, trueX)
    iOp2(optype::int\ <64>, trueX)
    iOp3(optype::*item-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <vector>>(op1->type());
        auto ty_op2 = as<type::int\ <64>>(op2->type());
        auto ty_op3 = as<type::*item-type*>(op3->type());

    }

    iDoc(R"(    
        Sets the element at index *op2* in vector *op1* to *op3.
    )")

iEnd

iBegin(vector, Size, "vector.size")
    iTarget(optype::int\ <64>)
    iOp1(optype::ref\ <vector>, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::ref\ <vector>>(op1->type());

    }

    iDoc(R"(    
        Returns the current size of the vector *op1*, which is the largest
        accessible index plus one.
    )")

iEnd

iBegin(vector, Timeout, "vector.timeout")
    iOp1(optype::ref\ <vector>, trueX)
    iOp2(optype::enum, trueX)
    iOp3(optype::interval, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <vector>>(op1->type());
        auto ty_op2 = as<type::enum>(op2->type());
        auto ty_op3 = as<type::interval>(op3->type());

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

