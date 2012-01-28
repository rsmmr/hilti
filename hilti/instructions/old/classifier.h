iBegin(classifier, Add, "classifier.add")
    iOp1(optype::ref\ <classifier>, trueX)
    iOp2(optype::*rule-with-optional-prio*, trueX)
    iOp3(optype::*value-type*, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <classifier>>(op1->type());
        auto ty_op2 = as<type::*rule-with-optional-prio*>(op2->type());
        auto ty_op3 = as<type::*value-type*>(op3->type());

    }

    iDoc(R"(    
        Adds a rule to classifier *op1*. The first element of *op2* is the
        rule's fields, and the second element is the rule's match priority. If
        multiple rules match with a later lookup, the rule with the highest
        priority will win. *op3* is the value that will be associated with the
        rule.  This instruction must not be called anymore after
        ``classifier.compile`` has been executed.
    )")

iEnd

iBegin(classifier, Compile, "classifier.compile")
    iOp1(optype::ref\ <classifier>, trueX)

    iValidate {
        auto ty_op1 = as<type::ref\ <classifier>>(op1->type());

    }

    iDoc(R"(    
        Freezes the classifier *op1* and prepares it for subsequent lookups.
    )")

iEnd

iBegin(classifier, Get, "classifier.get")
    iTarget(optype::*value-type*)
    iOp1(optype::ref\ <classifier>, trueX)
    iOp2(optype::*key-type*, trueX)

    iValidate {
        auto ty_target = as<type::*value-type*>(target->type());
        auto ty_op1 = as<type::ref\ <classifier>>(op1->type());
        auto ty_op2 = as<type::*key-type*>(op2->type());

    }

    iDoc(R"(    
        Returns the value associated with the highest-priority rule matching
        *op2* in classifier *op1*. Throws an IndexError exception if no match
        is found. If multiple rules of the same priority match, it is
        undefined which one will be selected.  This instruction must only be
        used after the classifier has been fixed with ``classifier.compile``.
    )")

iEnd

iBegin(classifier, Matches, "classifier.matches")
    iTarget(optype::bool)
    iOp1(optype::ref\ <classifier>, trueX)
    iOp2(optype::*key-type*, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::ref\ <classifier>>(op1->type());
        auto ty_op2 = as<type::*key-type*>(op2->type());

    }

    iDoc(R"(    
        Returns True if *op2* matches any rule in classifier *op1*.  This
        instruction must only be used after the classifier has been fixed with
        ``classifier.compile``.
    )")

iEnd

