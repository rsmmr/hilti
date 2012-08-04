
opBegin(integer::Plus)
    opOp1(std::make_shared<type::Integer>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Adds two integers.")

    opValidate() {
        auto t1 = ast::checkedCast<type::Integer>(op1()->type());
        auto t2 = ast::checkedCast<type::Integer>(op2()->type());

        if ( t1->width() != t2->width() )
            error(t1, "integer widths do not match");

        if ( t1->signed_() != t2->signed_() )
            error(t1, "sign mismatch for integers");
    }

    opResult() {
        auto t1 = ast::checkedCast<type::Integer>(op1()->type());
        auto t2 = ast::checkedCast<type::Integer>(op2()->type());

        return std::make_shared<type::Integer>(std::max(t1->width(), t2->width()), t1->signed_());
    }
opEnd

opBegin(integer::Minus)
    opOp1(std::make_shared<type::Integer>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Subtracts two integers.")

    opValidate() {
        auto t1 = ast::checkedCast<type::Integer>(op1()->type());
        auto t2 = ast::checkedCast<type::Integer>(op2()->type());

        if ( t1->width() != t2->width() )
            error(t1, "integer widths do not match");

        if ( t1->signed_() != t2->signed_() )
            error(t1, "sign mismatch for integers");
    }

    opResult() {
        auto t1 = ast::checkedCast<type::Integer>(op1()->type());
        auto t2 = ast::checkedCast<type::Integer>(op2()->type());

        return std::make_shared<type::Integer>(std::max(t1->width(), t2->width()), t1->signed_());
    }
opEnd







