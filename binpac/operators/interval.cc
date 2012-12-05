
using namespace binpac;

opBegin(interval::Equal)
    opOp1(std::make_shared<type::Interval>())
    opOp2(std::make_shared<type::Interval>())

    opDoc("Compares to intervals.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(interval::Lower)
    opOp1(std::make_shared<type::Interval>())
    opOp2(std::make_shared<type::Interval>())

    opDoc("Returns whether the first interval is smaller than the second.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(interval::Greater)
    opOp1(std::make_shared<type::Interval>())
    opOp2(std::make_shared<type::Interval>())

    opDoc("Returns whether the first interval is larger than the second.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(interval::Plus)
    opOp1(std::make_shared<type::Interval>())
    opOp2(std::make_shared<type::Interval>())

    opDoc("Adds two intervals.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Interval>();
    }
opEnd

opBegin(interval::Minus)
    opOp1(std::make_shared<type::Interval>())
    opOp2(std::make_shared<type::Interval>())

    opDoc("Subtracts two intervals.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Interval>();
    }
opEnd

opBegin(interval::MultInt1 : Mult)
    opOp1(std::make_shared<type::Interval>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Multiplies an interval with an integer.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Interval>();
    }
opEnd

opBegin(interval::MultInt2 : Mult)
    opOp1(std::make_shared<type::Integer>())
    opOp2(std::make_shared<type::Interval>())

    opDoc("Multiplies an integer with a interval.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Interval>();
    }
opEnd

opBegin(interval::CoerceBool : Coerce)
    opOp1(std::make_shared<type::Interval>())
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Bool>()))

    opDoc("Intervals coerce to boolean, returning true if the value is non-zero.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(interval::CastInteger : Cast)
    opOp1(std::make_shared<type::Interval>())
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Integer>()))

    opDoc("Casts a interval into an integer value, truncating any fractional value.")

    opValidate() {
    }

    opResult() {
        return ast::checkedCast<type::TypeType>(op2()->type())->typeType();
    }
opEnd

opBegin(interval::CastDouble : Cast)
    opOp1(std::make_shared<type::Interval>())
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Double>()))

    opDoc("Casts a interval into a double.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Double>();
    }
opEnd

opBegin(interval::Nsecs : MethodCall)
    opOp1(std::make_shared<type::Interval>())
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("nsecs")))

    opDoc("Returns the interval as nanoseconds.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd
