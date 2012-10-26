
opBegin(enum_::Call)
    opOp1(std::make_shared<type::TypeType>())
    opOp2(std::make_shared<type::Any>())

    opDoc("Converts an integer into an enum.")

    opMatch() {
        auto type = ast::checkedCast<type::TypeType>(op1()->type())->typeType();
        return ast::isA<type::Enum>(type);
    }

    opValidate() {
        auto ftype = ast::checkedCast<type::TypeType>(op1()->type());
    }

    opResult() {
        return ast::checkedCast<type::TypeType>(op1()->type())->typeType();
    }
opEnd
