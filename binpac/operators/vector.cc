
opBegin(vector::Index)
    opOp1(std::make_shared<type::Vector>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Returns the vector element at a given index.")

    opValidate() {
    }

    opResult() {
        auto v = ast::checkedCast<type::Vector>(op1()->type());
        return v->argType();
    }
opEnd

opBegin(vector::IndexAssign)
    opOp1(std::make_shared<type::Vector>())
    opOp2(std::make_shared<type::Integer>())
    opOp3(std::make_shared<type::Any>())

    opDoc("Assigns an element to the given index of the vector.")

    opValidate() {
        auto v = ast::checkedCast<type::Vector>(op1()->type());
        canCoerceTo(op3(), v->argType());
    }

    opResult() {
        auto v = ast::checkedCast<type::Vector>(op1()->type());
        return std::make_shared<type::Void>();
    }
opEnd

opBegin(vector::Size)
    opOp1(std::make_shared<type::Vector>())

    opDoc("Returns the length of the vector.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd

opBegin(vector::PushBack : MethodCall)
    opOp1(std::make_shared<type::Vector>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("push_back")))
    opCallArg1("elem", std::make_shared<type::Any>())

    opDoc("Appends an element to the vector.")

    opValidate() {
        // TODO: Check element ytpe.
    }

    opResult() {
        return op1()->type();
    }
opEnd

opBegin(vector::Reserve : MethodCall)
    opOp1(std::make_shared<type::Vector>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("reserve")))
    opCallArg1("c", std::make_shared<type::Integer>());

    opDoc("Resizes the vector to reserver a capacity of at least \a c. This shrinks the vector if \a c is smaller than the current size.")

    opValidate() {
        // TODO: Check element ytpe.
    }

    opResult() {
        return op1()->type();
    }
opEnd
