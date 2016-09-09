
#include "cg-operator-common.h"
#include <binpac/autogen/operators/integer.h>

using namespace binpac;
using namespace binpac::codegen;

static shared_ptr<binpac::type::Integer> _intResultType(binpac::expression::ResolvedOperator* op)
{
    auto t1 = ast::rtti::checkedCast<binpac::type::Integer>(op->op1()->type());
    auto t2 = ast::rtti::checkedCast<binpac::type::Integer>(op->op2()->type());

    int width = std::max(t1->width(), t2->width());
    bool sign = t1->signed_();

    // If one of the two is a constant, the other one determines the signedness and width.
    if ( op->op1()->isConstant() ) {
        sign = t2->signed_();
        width = t2->width();
    }

    else if ( op->op2()->isConstant() ) {
        sign = t1->signed_();
        width = t1->width();
    }

    else
        assert(t1->signed_() == t2->signed_());

    return std::make_shared<binpac::type::Integer>(width, sign);
}

void CodeBuilder::visit(constant::Integer* i)
{
    auto itype = ast::rtti::checkedCast<type::Integer>(i->type());
    auto result = cg()->hiltiConstantInteger(i->value(), itype->signed_());
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::CastTime* i)
{
    auto result = builder()->addTmp("i", hilti::builder::time::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::integer::AsTime, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::CastInterval* i)
{
    auto result = builder()->addTmp("i", hilti::builder::interval::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::integer::AsInterval, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::CoerceBool* i)
{
    auto result = builder()->addTmp("notnull", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto null = hilti::builder::integer::create((uint64_t)0);
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Unequal, op1, null);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::CoerceInteger* i)
{
    auto itype = ast::rtti::checkedCast<type::Integer>(i->type());
    auto optype = ast::rtti::checkedCast<type::Integer>(i->op1()->type());

    auto op1 = cg()->hiltiExpression(i->op1());

    if ( optype->width() == itype->width() || itype->wildcard() ) {
        setResult(op1);
        return;
    }

    assert(optype->width() < itype->width());

    auto result = builder()->addTmp("coerced", cg()->hiltiType(itype));

    if ( optype->signed_() )
        cg()->builder()->addInstruction(result, hilti::instruction::integer::SExt, op1);
    else
        cg()->builder()->addInstruction(result, hilti::instruction::integer::ZExt, op1);

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::CoerceInterval* i)
{
    auto result = builder()->addTmp("i", hilti::builder::interval::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::integer::AsInterval, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::CoerceDouble* i)
{
    auto result = builder()->addTmp("d", hilti::builder::double_::type());
    auto op1 = cg()->hiltiExpression(i->op1());

    if ( ast::rtti::checkedCast<type::Integer>(i->op1()->type())->signed_() )
        cg()->builder()->addInstruction(result, hilti::instruction::integer::AsSDouble, op1);
    else
        cg()->builder()->addInstruction(result, hilti::instruction::integer::AsUDouble, op1);

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::CastInteger* i)
{
    auto itype = ast::rtti::checkedCast<type::Integer>(i->type());
    auto optype = ast::rtti::checkedCast<type::Integer>(i->op1()->type());

    auto op1 = cg()->hiltiExpression(i->op1());

    if ( optype->width() == itype->width() ) {
        setResult(op1);
        return;
    }

    auto result = builder()->addTmp("casted", cg()->hiltiType(itype));

    if ( optype->width() < itype->width() ) {
        if ( optype->signed_() )
            cg()->builder()->addInstruction(result, hilti::instruction::integer::SExt, op1);
        else
            cg()->builder()->addInstruction(result, hilti::instruction::integer::ZExt, op1);
    }

    else
        cg()->builder()->addInstruction(result, hilti::instruction::integer::Trunc, op1);

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::Equal* i)
{
    auto result = builder()->addTmp("equal", hilti::builder::boolean::type());
    auto itype = _intResultType(i);
    auto op1 = cg()->hiltiExpression(i->op1(), itype);
    auto op2 = cg()->hiltiExpression(i->op2(), itype);
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Equal, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::Lower* i)
{
    auto t = ast::rtti::checkedCast<type::Integer>(i->op1()->type());

    auto result = builder()->addTmp("lt", hilti::builder::boolean::type());
    auto itype = _intResultType(i);
    auto op1 = cg()->hiltiExpression(i->op1(), itype);
    auto op2 = cg()->hiltiExpression(i->op2(), itype);

    if ( t->signed_() )
        cg()->builder()->addInstruction(result, hilti::instruction::integer::Slt, op1, op2);
    else
        cg()->builder()->addInstruction(result, hilti::instruction::integer::Ult, op1, op2);

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::Greater* i)
{
    auto t = ast::rtti::checkedCast<type::Integer>(i->op1()->type());

    auto result = builder()->addTmp("gt", hilti::builder::boolean::type());
    auto itype = _intResultType(i);
    auto op1 = cg()->hiltiExpression(i->op1(), itype);
    auto op2 = cg()->hiltiExpression(i->op2(), itype);

    if ( t->signed_() )
        cg()->builder()->addInstruction(result, hilti::instruction::integer::Sgt, op1, op2);
    else
        cg()->builder()->addInstruction(result, hilti::instruction::integer::Ugt, op1, op2);

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::Minus* i)
{
    auto rtype = _intResultType(i);
    auto result = builder()->addTmp("diff", cg()->hiltiType(rtype));
    auto op1 = cg()->hiltiExpression(i->op1(), rtype);
    auto op2 = cg()->hiltiExpression(i->op2(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Sub, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::MinusAssign* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2(), i->op1()->type());
    cg()->builder()->addInstruction(op1, hilti::instruction::integer::DecrBy, op1, op2);
    setResult(op1);
}

void CodeBuilder::visit(expression::operator_::integer::Plus* i)
{
    auto rtype = _intResultType(i);
    auto result = builder()->addTmp("sum", cg()->hiltiType(rtype));
    auto op1 = cg()->hiltiExpression(i->op1(), rtype);
    auto op2 = cg()->hiltiExpression(i->op2(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Add, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::PlusAssign* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2(), i->op1()->type());
    cg()->builder()->addInstruction(op1, hilti::instruction::integer::IncrBy, op1, op2);
    setResult(op1);
}

void CodeBuilder::visit(expression::operator_::integer::Mult* i)
{
    auto rtype = _intResultType(i);
    auto result = builder()->addTmp("prod", cg()->hiltiType(rtype));
    auto op1 = cg()->hiltiExpression(i->op1(), rtype);
    auto op2 = cg()->hiltiExpression(i->op2(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Mul, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::Div* i)
{
    auto rtype = _intResultType(i);
    auto result = builder()->addTmp("div", cg()->hiltiType(rtype));
    auto op1 = cg()->hiltiExpression(i->op1(), rtype);
    auto op2 = cg()->hiltiExpression(i->op2(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Div, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::Mod* i)
{
    auto rtype = _intResultType(i);
    auto result = builder()->addTmp("rem", cg()->hiltiType(rtype));
    auto op1 = cg()->hiltiExpression(i->op1(), rtype);
    auto op2 = cg()->hiltiExpression(i->op2(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Mod, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::Power* i)
{
    auto result = builder()->addTmp("pow", cg()->hiltiType(i->op1()->type()));
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Pow, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::BitAnd* i)
{
    auto rtype = _intResultType(i);
    auto result = builder()->addTmp("and", cg()->hiltiType(rtype));
    auto op1 = cg()->hiltiExpression(i->op1(), rtype);
    auto op2 = cg()->hiltiExpression(i->op2(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::integer::And, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::BitOr* i)
{
    auto rtype = _intResultType(i);
    auto result = builder()->addTmp("or", cg()->hiltiType(rtype));
    auto op1 = cg()->hiltiExpression(i->op1(), rtype);
    auto op2 = cg()->hiltiExpression(i->op2(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Or, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::BitXor* i)
{
    auto rtype = _intResultType(i);
    auto result = builder()->addTmp("xor", cg()->hiltiType(rtype));
    auto op1 = cg()->hiltiExpression(i->op1(), rtype);
    auto op2 = cg()->hiltiExpression(i->op2(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Xor, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::ShiftLeft* i)
{
    auto rtype = _intResultType(i);
    auto result = builder()->addTmp("shl", cg()->hiltiType(rtype));
    auto op1 = cg()->hiltiExpression(i->op1(), rtype);
    auto op2 = cg()->hiltiExpression(i->op2(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Shl, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::ShiftRight* i)
{
    auto rtype = _intResultType(i);
    auto result = builder()->addTmp("shr", cg()->hiltiType(rtype));
    auto op1 = cg()->hiltiExpression(i->op1(), rtype);
    auto op2 = cg()->hiltiExpression(i->op2(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Shr, op1, op2);
    setResult(result);
}
