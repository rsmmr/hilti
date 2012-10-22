
opBegin(tuple::Equal)
    opOp1(std::make_shared<type::Tuple>())
    opOp2(std::make_shared<type::Tuple>())

    opDoc("Compares two tuples for equality.")

    opValidate() {
        auto types1 = ast::checkedCast<type::Tuple>(op1()->type())->typeList();
        auto types2 = ast::checkedCast<type::Tuple>(op2()->type())->typeList();

        if ( types1.size() != types2.size() ) {
            error(op1(), "incompatible tuple types");
            return;
        }

        auto i1 = types1.begin();
        auto i2 = types2.begin();

        while ( i1 != types1.end() ) {
            if ( ! Coercer().canCoerceTo(*i1, *i2) && ! Coercer().canCoerceTo(*i2, *i1) ) {
                error(*i1, "incompatible tuple elements");
                return;
            }

            ++i1;
            ++i2;
        }
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(tuple::Index)
    opOp1(std::make_shared<type::Tuple>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Returns the tuple element at a given index.")

    opValidate() {
        auto tuple = ast::checkedCast<type::Tuple>(op1()->type());
        auto idx = ast::tryCast<expression::Constant>(op2());

        if ( ! idx ) {
            error(op2(), "tuple index must a be a constant");
        }
    }

    opResult() {
        auto tuple = ast::checkedCast<type::Tuple>(op1()->type());
        auto types = tuple->typeList();

        auto idx = ast::checkedCast<expression::Constant>(op2());
        auto const_ = ast::checkedCast<constant::Integer>(idx->constant());

        auto i = types.begin();
        std::advance(i, const_->value());
        return (*i);
    }
opEnd
