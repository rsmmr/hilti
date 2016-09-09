
using namespace binpac;

opBegin(time::Equal)
    opOp1(std::make_shared<type::Time>());
    opOp2(std::make_shared<type::Time>());

    opDoc("Compares to times.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(time::Lower)
    opOp1(std::make_shared<type::Time>());
    opOp2(std::make_shared<type::Time>());

    opDoc("Returns whether the first time is smaller than the second.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(time::Greater)
    opOp1(std::make_shared<type::Time>());
    opOp2(std::make_shared<type::Time>());

    opDoc("Returns whether the first time is larger than the second.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(time::Minus)
    opOp1(std::make_shared<type::Time>());
    opOp2(std::make_shared<type::Time>());

    opDoc("Subtracts two times.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Interval>();
    }
opEnd

opBegin(time::MinusInterval : Minus)
    opOp1(std::make_shared<type::Time>());
    opOp2(std::make_shared<type::Interval>());

    opDoc("Subtracts an interval from a time.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Interval>();
    }
opEnd

opBegin(time::PlusInterval1 : Plus)
    opOp1(std::make_shared<type::Time>());
    opOp2(std::make_shared<type::Interval>());

    opDoc("Adds an interval to a time.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Time>();
    }
opEnd

opBegin(time::PlusInterval2 : Plus)
    opOp1(std::make_shared<type::Interval>());
    opOp2(std::make_shared<type::Time>());

    opDoc("Adds an interval to a time.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Time>();
    }
opEnd

opBegin(time::CoerceBool : Coerce)
    opOp1(std::make_shared<type::Time>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Bool>()));

    opDoc("Times coerce to boolean, returning true if the value is non-zero.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(time::CastInteger : Cast)
    opOp1(std::make_shared<type::Time>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Integer>()));

    opDoc("Casts a time into an integer value, truncating any fractional value.");

    opValidate()
    {
    }

    opResult()
    {
        return ast::rtti::checkedCast<type::TypeType>(op2()->type())->typeType();
    }
opEnd

opBegin(time::CastDouble : Cast)
    opOp1(std::make_shared<type::Time>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Double>()));

    opDoc("Casts a time into a double.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Double>();
    }
opEnd

opBegin(time::Nsecs : MethodCall)
    opOp1(std::make_shared<type::Time>());
    opOp2(std::make_shared<type::MemberAttribute>(std::make_shared<ID>("nsecs")));

    opDoc("Returns the time as nanoseconds.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Integer>(64, false);
    }
opEnd
