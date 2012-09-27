
opBegin(list::PushBack : MethodCall)
    opOp1(std::make_shared<type::List>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("push_back")))
    opOp3(std::make_shared<type::Tuple>())

    opDoc("Appends an element to the list.")

    opValidate() {
        auto list = ast::checkedCast<type::List>(op1()->type());
        auto tuple = ast::checkedCast<type::List>(op3()->type());
        // TODO: Check element ytpe.
    }

    opResult() {
        return op1()->type();
    }
opEnd
