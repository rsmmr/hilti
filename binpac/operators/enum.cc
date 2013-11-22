
opBegin(enum_::Equal)
    opOp1(std::make_shared<type::Enum>())
    opOp2(std::make_shared<type::Enum>())

    opDoc("Compared two boolean values.")

    opValidate() {
        sameType(op1()->type(), op2()->type());
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(enum_::Call)
    opOp1(std::make_shared<type::TypeType>())
    opOp2(std::make_shared<type::Any>())

    opDoc("Converts an integer into an enum.")

    opMatch() {
        auto type = ast::tryCast<type::TypeType>(op1()->type());
        return type && ast::isA<type::Enum>(type->typeType());
    }

    opResult() {
        return ast::checkedCast<type::TypeType>(op1()->type())->typeType();
    }
opEnd
