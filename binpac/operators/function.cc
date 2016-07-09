
opBegin(function::Call)
    opOp1(std::make_shared<type::Function>());
    opOp2(std::make_shared<type::Any>());

    opDoc("Calls a function.");

    opValidate()
    {
        auto ftype = ast::checkedCast<type::Function>(op1()->type());

        // TODO: Check signature.
    }

    opResult()
    {
        auto ftype = ast::checkedCast<type::Function>(op1()->type());
        return ftype->result() ? ftype->result()->type() : std::make_shared<type::Void>();
    }
opEnd
