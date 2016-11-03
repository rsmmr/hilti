
using namespace spicy;

static bool _checkOperands(Operator* op, shared_ptr<Expression> op1, shared_ptr<Expression> op2)
{
    auto t1 = ast::rtti::checkedCast<type::Integer>(op1->type());
    auto t2 = ast::rtti::checkedCast<type::Integer>(op2->type());

    // Check if one can be coerced into the other.
    if ( ! (op1->canCoerceTo(t2) || op2->canCoerceTo(t1)) ) {
        op->error(op1, "incompatible integer operands");
        return false;
    }

    return true;
}

static shared_ptr<Type> _resultType(shared_ptr<Expression> op1, shared_ptr<Expression> op2)
{
    assert(op1->type());
    assert(op2->type());
    auto t1 = ast::rtti::checkedCast<type::Integer>(op1->type());
    auto t2 = ast::rtti::checkedCast<type::Integer>(op2->type());

    shared_ptr<Type> result = nullptr;

    if ( op1->canCoerceTo(t2) )
        return op1->coerceTo(t2)->type();

    if ( op2->canCoerceTo(t1) )
        return op2->coerceTo(t1)->type();

    // Just return one, the validator will catch the error later.
    return op1->type();
}

opBegin(integer::CastTime : Cast)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Time>()));

    opDoc(
        "Casts an unsigned integer into a time, interpreting the value as seconds since the "
        "epoch.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Time>();
    }
opEnd

opBegin(integer::CastInterval : Cast)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Interval>()));

    opDoc("Casts an unsigned integer into an interval, interpreting the value as seconds.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Interval>();
    }
opEnd

opBegin(integer::Equal)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Compares two integer for equality.");

    opValidate()
    {
        _checkOperands(this, op1(), op2());
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd


opBegin(integer::Lower)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Returns whether the first integer is smaller than the second.");

    opValidate()
    {
        _checkOperands(this, op1(), op2());
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(integer::Greater)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Returns whether the first integer is larger than the second.");

    opValidate()
    {
        _checkOperands(this, op1(), op2());
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(integer::Plus)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Adds two integers.");

    opValidate()
    {
        _checkOperands(this, op1(), op2());
    }

    opResult()
    {
        return _resultType(op1(), op2());
    }
opEnd

opBegin(integer::PlusAssign)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Increases an integer by a given amount.");

    opValidate()
    {
    }

    opResult()
    {
        return op1()->type();
    }
opEnd

opBegin(integer::Minus)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Subtracts two integers.");

    opValidate()
    {
        _checkOperands(this, op1(), op2());
    }

    opResult()
    {
        return _resultType(op1(), op2());
    }

opEnd

opBegin(integer::MinusAssign)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Decreases an integer by a given amount.");

    opValidate()
    {
    }

    opResult()
    {
        return op1()->type();
    }
opEnd

opBegin(integer::Div)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Divides two integers.");

    opValidate()
    {
        _checkOperands(this, op1(), op2());

        auto c = ast::rtti::tryCast<expression::Constant>(op2());
        if ( c ) {
            auto i = ast::rtti::tryCast<constant::Integer>(c->constant());
            if ( i && i->value() == 0 )
                error(op1(), "division by zero");
        }
    }

    opResult()
    {
        return _resultType(op1(), op2());
    }
opEnd

opBegin(integer::Mult)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Multiplies two integers.");

    opValidate()
    {
        _checkOperands(this, op1(), op2());
    }

    opResult()
    {
        return _resultType(op1(), op2());
    }
opEnd

opBegin(integer::Mod)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Returns the remainder of a integers' division.");

    opValidate()
    {
        _checkOperands(this, op1(), op2());

        auto c = ast::rtti::tryCast<expression::Constant>(op2());
        if ( c ) {
            auto i = ast::rtti::tryCast<constant::Integer>(c->constant());
            if ( i && i->value() == 0 )
                error(op1(), "division by zero");
        }
    }

    opResult()
    {
        return _resultType(op1(), op2());
    }
opEnd

opBegin(integer::Power)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Raises an integer to a given power.");

    opValidate()
    {
    }

    opResult()
    {
        return op1()->type();
    }
opEnd

opBegin(integer::BitAnd)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Computes the bitwise \a and of two integers.");

    opValidate()
    {
        _checkOperands(this, op1(), op2());
    }

    opResult()
    {
        return _resultType(op1(), op2());
    }
opEnd

opBegin(integer::BitOr)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Computes the bitwise \a or of two integers.");

    opValidate()
    {
        _checkOperands(this, op1(), op2());
    }

    opResult()
    {
        return _resultType(op1(), op2());
    }
opEnd

opBegin(integer::BitXor)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Computes the bitwise \a xor of two integers.");

    opValidate()
    {
        _checkOperands(this, op1(), op2());
    }

    opResult()
    {
        return _resultType(op1(), op2());
    }
opEnd

opBegin(integer::ShiftLeft)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Shifts an integer left by a given number of bits.");

    opValidate()
    {
    }

    opResult()
    {
        return op1()->type();
    }
opEnd

opBegin(integer::ShiftRight)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::Integer>());

    opDoc("Shifts an integer right by a given number of bits.");

    opValidate()
    {
    }

    opResult()
    {
        return op1()->type();
    }
opEnd

opBegin(integer::CoerceBool : Coerce)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Bool>()));

    opDoc("Integers coerce to boolean, returning true if the value is non-zero.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(integer::CoerceInteger : Coerce)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Integer>()));

    opDoc(
        "Integers coerce to other integer types if their signedness match and their width is "
        "larger or equal.");

    opMatch()
    {
        auto t1 = ast::rtti::checkedCast<type::Integer>(op1()->type());
        auto ttype = ast::rtti::checkedCast<type::TypeType>(op2()->type())->typeType();
        auto t2 = ast::rtti::checkedCast<type::Integer>(ttype);

        if ( (t1->signed_() && ! t2->signed_()) || (! t1->signed_() && t2->signed_()) )
            return false;

        if ( t1->width() > t2->width() )
            return false;

        return true;
    }

    opValidate()
    {
    }

    opResult()
    {
        auto ttype = ast::rtti::checkedCast<type::TypeType>(op2()->type())->typeType();
        return ast::rtti::checkedCast<type::Integer>(ttype);
    }
opEnd

opBegin(integer::CoerceInterval : Coerce)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Interval>()));

    opDoc("Unsigned integers coerce to intervals.");

    opMatch()
    {
        auto t1 = ast::rtti::checkedCast<type::Integer>(op1()->type());
        return ! t1->signed_();
    }

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Interval>();
    }
opEnd

opBegin(integer::CoerceDouble : Coerce)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Double>()));

    opDoc("Unsigned integers coerce to doubles.");

    opValidate()
    {
    }

    opResult()
    {
        return std::make_shared<type::Double>();
    }
opEnd


opBegin(integer::CastInteger : Cast)
    opOp1(std::make_shared<type::Integer>());
    opOp2(std::make_shared<type::TypeType>(std::make_shared<type::Integer>()));

    opDoc("Casts an integer into a different integer type, extending/truncating as needed.");

    opValidate()
    {
    }

    opResult()
    {
        auto ttype = ast::rtti::checkedCast<type::TypeType>(op2()->type())->typeType();
        return ast::rtti::checkedCast<type::Integer>(ttype);
    }
opEnd
