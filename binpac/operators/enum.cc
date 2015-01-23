
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

opBegin(enum_::CastInteger : Cast)
    opOp1(std::make_shared<type::Enum>())
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Integer>()))

    opDoc("Casts an enum into an integer, returning a value that is consistent and unique among all labels of the enum's type.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>();
    }
opEnd

opBegin(enum_::CoerceBool : Coerce)
    opOp1(std::make_shared<type::Enum>())
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Bool>()))

    opDoc("Enums coerce to boolean, returning true if the value corresponds to a known label.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

