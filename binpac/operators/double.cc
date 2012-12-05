
using namespace binpac;

opBegin(double_::Equal)
    opOp1(std::make_shared<type::Double>())
    opOp2(std::make_shared<type::Double>())

    opDoc("Compares to doubles.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(double_::Lower)
    opOp1(std::make_shared<type::Double>())
    opOp2(std::make_shared<type::Double>())

    opDoc("Returns whether the first double is smaller than the second.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(double_::Greater)
    opOp1(std::make_shared<type::Double>())
    opOp2(std::make_shared<type::Double>())

    opDoc("Returns whether the first double is larger than the second.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(double_::Plus)
    opOp1(std::make_shared<type::Double>())
    opOp2(std::make_shared<type::Double>())

    opDoc("Adds two doubles.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Double>();
    }
opEnd

opBegin(double_::Minus)
    opOp1(std::make_shared<type::Double>())
    opOp2(std::make_shared<type::Double>())

    opDoc("Subtracts two doubles.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Double>();
    }
opEnd

opBegin(double_::Div)
    opOp1(std::make_shared<type::Double>())
    opOp2(std::make_shared<type::Double>())

    opDoc("Divides two doubles.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Double>();
    }
opEnd

opBegin(double_::Mult)
    opOp1(std::make_shared<type::Double>())
    opOp2(std::make_shared<type::Double>())

    opDoc("Multiplies two doubles.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Double>();
    }
opEnd

opBegin(double_::Mod)
    opOp1(std::make_shared<type::Double>())
    opOp2(std::make_shared<type::Double>())

    opDoc("Returns the remainder of a doubles' division.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Double>();
    }
opEnd

opBegin(double_::Power)
    opOp1(std::make_shared<type::Double>())
    opOp2(std::make_shared<type::Double>())

    opDoc("Raises a double to a given power.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Double>();
    }
opEnd

opBegin(double_::CoerceBool : Coerce)
    opOp1(std::make_shared<type::Double>())
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Bool>()))

    opDoc("Doubles coerce to boolean, returning true if the value is non-zero.")

    opValidate() {
    }

    opResult() {
        return std::make_shared<type::Double>();
    }
opEnd

// opBegin(double_::CoerceInterval : Coerce)
//     opOp1(std::make_shared<type::Double>())
//     opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Interval>()))
// 
//  opDoc("Doubles coerce to intervals, but they must be non-negative.")
// 
//  opValidate() {
//     }
// 
//     opResult() {
//         return std::make_shared<type::Interval>();
//     }
// opEnd

opBegin(double_::CastInteger : Cast)
    opOp1(std::make_shared<type::Double>())
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Integer>()))

    opDoc("Casts a double into an integer value, truncating any fractional value.")

    opValidate() {
    }

    opResult() {
        return ast::checkedCast<type::TypeType>(op2()->type())->typeType();
    }
opEnd
