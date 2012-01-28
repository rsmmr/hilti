iBegin(list, Back, "list.back")
    iTarget(optype::*item-type*)
    iOp1(optype::ref\ <list>, trueX)

    iValidate {
        auto ty_target = as<type::*item-type*>(target->type());
        auto ty_op1 = as<type::ref\ <list>>(op1->type());

    }

    iDoc(R"(    
        Returns the last element from the list referenced by *op1*. If the
        list is empty, raises an ~~Underflow exception.
    )")

iEnd

iBegin(list, Erase, "list.erase")
    iOp1(optype::iterator<list\<T>>, trueX)

    iValidate {
        auto ty_op1 = as<type::iterator<list\<T>>>(op1->type());

    }

    iDoc(R"(    
        Removes the list element located by the iterator in *op1*.
    )")

iEnd

iBegin(list, Front, "list.front")
    iTarget(optype::*item-type*)
    iOp1(optype::ref\ <list>, trueX)

    iValidate {
        auto ty_target = as<type::*item-type*>(target->type());
        auto ty_op1 = as<type::ref\ <list>>(op1->type());

    }

    iDoc(R"(    
        Returns the first element from the list referenced by *op1*. If the
        list is empty, raises an ~~Underflow exception.
    )")

iEnd

iBegin(list, Insert, "list.insert")
    iOp1(optype::*item-type*, trueX)
    iOp2(optype::iterator<list\<T>>, trueX)

    iValidate {
        auto ty_op1 = as<type::*item-type*>(op1->type());
        auto ty_op2 = as<type::iterator<list\<T>>>(op2->type());

    }

    iDoc(R"(    
        Inserts the element *op1* into a list before the element located by
        the iterator in *op2*. If the iterator is at the end position, the
        element will become the new list tail.
    )")

iEnd

iBegin(list, PopBack, "list.pop_back")
    iTarget(optype::*item-type*)
    iOp1(optype::ref\ <list>, trueX)

    iValidate {
        auto ty_target = as<type::*item-type*>(target->type());
        auto ty_op1 = as<type::ref\ <list>>(op1->type());

    }

    iDoc(R"(    
        Removes the last element from the list referenced by *op1* and returns
        it. If the list is empty, raises an ~~Underflow exception.
    )")

iEnd

iBegin(list, PopFront, "list.pop_front")
    iTarget(optype::*item-type*)
    iOp1(optype::ref\ <list>, trueX)

    iValidate {
        auto ty_target = as<type::*item-type*>(target->type());
        auto ty_op1 = as<type::ref\ <list>>(op1->type());

    }

    iDoc(R"(    
        Removes the first element from the list referenced by *op1* and
        returns it. If the list is empty, raises an ~~Underflow exception.
    )")

iEnd

iBegin(list, PushBack, "list.push_back")
    iOp1(optype::ref\ <list>, trueX)
    iOp2(optype::*item-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <list>>(op1->type());
        auto ty_op2 = as<type::*item-type*>(op2->type());

    }

    iDoc(R"(    
        Appends *op2* to the list in reference by *op1*.
    )")

iEnd

iBegin(list, PushFront, "list.push_front")
    iOp1(optype::ref\ <list>, trueX)
    iOp2(optype::*item-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <list>>(op1->type());
        auto ty_op2 = as<type::*item-type*>(op2->type());

    }

    iDoc(R"(    
        Prepends *op2* to the list in reference by *op1*.
    )")

iEnd

iBegin(list, Size, "list.size")
    iTarget(optype::int\ <64>)
    iOp1(optype::ref\ <list>, trueX)

    iValidate {
        auto ty_target = as<type::int\ <64>>(target->type());
        auto ty_op1 = as<type::ref\ <list>>(op1->type());

    }

    iDoc(R"(    
        Returns the current size of the list reference by *op1*.
    )")

iEnd

iBegin(list, Timeout, "list.timeout")
    iOp1(optype::ref\ <list>, trueX)
    iOp2(optype::enum, trueX)
    iOp3(optype::interval, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <list>>(op1->type());
        auto ty_op2 = as<type::enum>(op2->type());
        auto ty_op3 = as<type::interval>(op3->type());

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

