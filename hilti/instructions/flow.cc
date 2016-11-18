
// Instructions controlling flow.

static void _validateBranches(const Instruction* ins, shared_ptr<Expression> op,
                              shared_ptr<Type> ty_op1, bool by_type)
{
    ins->isConstant(op);

    auto a1 = ast::rtti::tryCast<expression::Constant>(op);
    auto a2 = ast::rtti::tryCast<constant::Tuple>(a1->constant());

    for ( auto i : a2->value() ) {
        auto t = ast::rtti::tryCast<type::Tuple>(i->type());

        if ( ! t ) {
            ins->error(i, "not a tuple");
            return;
        }

        if ( t->typeList().size() != 2 ) {
            ins->error(i, "clause must be a tuple (value, label)");
            return;
        }

        ins->isConstant(i);

        auto c1 = ast::rtti::tryCast<expression::Constant>(i);
        auto c2 = ast::rtti::tryCast<constant::Tuple>(c1->constant());

        auto list = c2->value();
        auto j = list.begin();
        auto t1 = *j++;
        auto t2 = *j++;

        if ( ! by_type ) {
            ins->canCoerceTo(t1, ty_op1);
            ins->equalTypes(t2->type(), builder::label::type());
        }

        else {
            auto t = ast::rtti::tryCast<expression::Type>(t1);

            if ( ! t ) {
                ins->error(t1, "not a type");
                return;
            }

            auto types = ast::rtti::checkedCast<type::Union>(ty_op1)->fields(t->typeValue());
            if ( ! types.size() )
                ins->error(t1, "type does not exist in union");
        }
    }
}

static std::set<shared_ptr<Expression>> _successorBranches(const Instruction* ins,
                                                           shared_ptr<Expression> op,
                                                           shared_ptr<Expression> def)
{
    std::set<shared_ptr<Expression>> succ = {def};

    auto a1 = ast::rtti::tryCast<expression::Constant>(op);
    auto a2 = ast::rtti::tryCast<constant::Tuple>(a1->constant());

    for ( auto i : a2->value() ) {
        auto t = ast::rtti::tryCast<type::Tuple>(i->type());
        auto c1 = ast::rtti::tryCast<expression::Constant>(i);
        auto c2 = ast::rtti::tryCast<constant::Tuple>(c1->constant());

        auto list = c2->value();
        auto j = list.begin();
        auto t1 = *j++;
        auto t2 = *j++;

        succ.insert(t2);
    }

    return succ;
}

iBegin(flow::ReturnResult, "return.result")
    iTerminator();
    iOp1(optype::any, false);

    iValidate
    {
        auto decl = validator()->current<Declaration>();

        if ( decl && ast::rtti::isA<declaration::Hook>(decl) ) {
            error(nullptr, "return.result must not be used inside a hook; use hook.stop instead");
            return;
        }
    }

    iSuccessors
    {
        return std::set<shared_ptr<Expression>>();
    }

    iDoc(R"(
    )");
iEnd

iBegin(flow::ReturnVoid, "return.void")
    iTerminator();

    iValidate
    {
    }

    iSuccessors
    {
        return std::set<shared_ptr<Expression>>();
    }

    iDoc(R"(
    )");
iEnd

iBegin(flow::BlockEnd, "block.end")
    iTerminator();

    iSuccessors
    {
        return std::set<shared_ptr<Expression>>();
    }

    iDoc(R"(
        Internal instruction marking the end of a block that doesn't have any other terminator.
    )");
iEnd

iBegin(flow::CallVoid, "call")
    iOp1(optype::function, true);
    iOp2(optype::tuple, false);

    iValidate
    {
        auto ftype = ast::rtti::checkedCast<type::Function>(op1->type());
        auto rtype = ftype->result()->type();
        shared_ptr<Type> none = nullptr;
        checkCallParameters(ftype, op2);
        checkCallResult(rtype, none);
    }

    iDoc(R"(
    )");
iEnd

iBegin(flow::CallResult, "call")
    iTarget(optype::any);
    iOp1(optype::function, true);
    iOp2(optype::tuple, false);

    iValidate
    {
        auto ftype = ast::rtti::checkedCast<type::Function>(op1->type());
        auto rtype = ftype->result()->type();
        checkCallParameters(ftype, op2);
        checkCallResult(rtype, target->type());
    }

    iDoc(R"(
    )");
iEnd

iBegin(flow::CallCallableResult, "call")
    iTarget(optype::any);
    iOp1(optype::refCallable, true);
    iOp2(optype::optional(optype::tuple), false);

    iValidate
    {
        auto rt = ast::rtti::checkedCast<type::Reference>(op1->type());
        auto ftype = ast::rtti::checkedCast<type::Callable>(rt->argType());
        auto rtype = ftype->result()->type();
        checkCallParameters(ftype, op2);
        checkCallResult(rtype, target->type());
    }

    iDoc(R"(
    )");
iEnd

iBegin(flow::CallCallableVoid, "call")
    iOp1(optype::refCallable, true);
    iOp2(optype::optional(optype::tuple), false);

    iValidate
    {
        auto rt = ast::rtti::checkedCast<type::Reference>(op1->type());
        auto ftype = ast::rtti::checkedCast<type::Callable>(rt->argType());
        auto rtype = ftype->result()->type();
        shared_ptr<Type> none = nullptr;
        checkCallParameters(ftype, op2);
        checkCallResult(rtype, none);
    }


    iDoc(R"(
    )");
iEnd

iBegin(flow::Yield, "yield")
    iValidate
    {
    }

    iDoc(R"(
        Yields processing back to the current scheduler, to be resumed later.
        If running in a virtual thread other than zero, this instruction yields
        to other virtual threads running within the same physical thread. If
        running in virtual thread zero (or in non-threading mode), returns
        execution back to the calling C function (see interfacing with C).
    )");
iEnd

iBegin(flow::YieldUntil, "yield.until")
    iOp1(optype::any, false);

    iValidate
    {
        shared_ptr<Type> ty = op1->type();
        auto rtype = ast::rtti::tryCast<type::Reference>(ty);

        if ( rtype )
            ty = rtype->argType();

        if ( ! ast::type::hasTrait<type::trait::Blockable>(ty) )
            error(op1, "operand type does not support yield.until");
    }

    iDoc(R"(
        )");
iEnd

iBegin(flow::IfElse, "if.else")
    iTerminator();
    iOp1(optype::boolean, true);
    iOp2(optype::label, true);
    iOp3(optype::label, true);

    iSuccessors
    {
        return {op2, op3};
    }

    iValidate
    {
    }

    iDoc(R"(
        Transfers control label *op2* if *op1* is true, and to *op3* otherwise.
    )");
iEnd

iBegin(flow::Jump, "jump")
    iTerminator();
    iOp1(optype::label, true);

    iValidate
    {
    }

    iSuccessors
    {
        return {op1};
    }

    iDoc(R"(
        Jumps unconditionally to label *op2*.
    )");
iEnd

iBegin(flow::Switch, "switch")
    iTerminator();
    iOp1(optype::any, true);
    iOp2(optype::label, true);
    iOp3(optype::tuple, true);

    iValidate
    {
        auto ty_op1 = op1->type();
        auto ty_op2 = ast::rtti::checkedCast<type::Label>(op2->type());
        auto ty_op3 = ast::rtti::checkedCast<type::Tuple>(op3->type());

        if ( ! ast::type::hasTrait<type::trait::ValueType>(ty_op1) ) {
            error(op1, "switch operand must be a value type");
            return;
        }

        _validateBranches(this, op3, ty_op1, false);
    }

    iSuccessors
    {
        return _successorBranches(this, op3, op2);
    }

    iDoc(R"(
        Branches to one of several alternatives. *op1* determines which
        alternative to take. *op3* is a tuple of giving all alternatives as
        2-tuples *(value, destination)*. *value* must be of the same type
        as *op1*, and *destination* is a block label. If *value* equals
        *op1*, control is transfered to the corresponding block. If multiple
        alternatives match *op1*, one of them is taken but it's undefined which
        one. If no alternative matches, control is transfered to block *op2*.
    )");

iEnd

iBegin(flow::DispatchUnion, "dispatch")
    iTerminator();
    iOp1(optype::union_, true);
    iOp2(optype::label, true);
    iOp3(optype::tuple, true);

    iValidate
    {
        auto ty_op1 = op1->type();
        auto ty_op2 = ast::rtti::checkedCast<type::Label>(op2->type());
        auto ty_op3 = ast::rtti::checkedCast<type::Tuple>(op3->type());

        _validateBranches(this, op3, ty_op1, true);
    }

    iSuccessors
    {
        return _successorBranches(this, op3, op2);
    }

    iDoc(R"(
        Branches to one of several alternatives. The type of the current
        value of union *op1* determines which alternative to take. *op3* is a
        tuple of giving all alternatives as 2-tuples *(type, destination)*.
        *type* must be one of the types that the union *op1* defines. and
        *destination* is a block label. If *type* equals that of the current
        value of *op1*, control is transfered to the corresponding block. If
        multiple alternatives match the type, one of them is taken but it's
        undefined which one. If no alternative matches (including when no unit
        field is set), control is transfered to block *op2*.
    )");
iEnd
