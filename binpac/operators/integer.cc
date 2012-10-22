
using namespace binpac;

static bool _checkOperands(Operator* op, shared_ptr<Expression> op1, shared_ptr<Expression> op2)
{
    auto t1 = ast::checkedCast<type::Integer>(op1->type());
    auto t2 = ast::checkedCast<type::Integer>(op2->type());

    if ( t1->signed_() != t2->signed_() ) {
        op->error(t1, "sign mismatch between operands");
        return false;
    }

    return true;
}

static shared_ptr<Type> _resultType(shared_ptr<Expression> op1, shared_ptr<Expression> op2)
{
    auto t1 = ast::checkedCast<type::Integer>(op1->type());
    auto t2 = ast::checkedCast<type::Integer>(op2->type());
    return std::make_shared<type::Integer>(std::max(t1->width(), t2->width()), t1->signed_(), op1->location());
}

opBegin(integer::Plus)
    opOp1(std::make_shared<type::Integer>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Adds two integers.")

    opValidate() {
        _checkOperands(this, op1(), op2());
    }

    opResult() {
        return _resultType(op1(), op2());
    }
opEnd

opBegin(integer::Minus)
    opOp1(std::make_shared<type::Integer>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Subtracts two integers.")

    opValidate() {
        _checkOperands(this, op1(), op2());
    }

    opResult() {
        return _resultType(op1(), op2());
    }

opEnd

opBegin(integer::Equal)
    opOp1(std::make_shared<type::Integer>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Compares two integer for equality.")

    opValidate() {
        _checkOperands(this, op1(), op2());
    }

    opResult() {
        return std::make_shared<type::Bool>();
    }
opEnd

opBegin(integer::Attribute)
    opOp1(std::make_shared<type::Integer>())
    opOp2(std::make_shared<type::MemberAttribute>())

    opDoc("Access a bitfield element.")

    opValidate() {
        auto itype = ast::checkedCast<type::Integer>(op1()->type());
        auto attr = ast::checkedCast<expression::MemberAttribute>(op2());

        if ( ! itype->bits(attr->id()) )
            error(op2(), "unknown bitfield element");
    }

    opResult() {
        return op1()->type();
    }
opEnd
