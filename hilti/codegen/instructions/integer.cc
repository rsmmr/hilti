
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::integer::Equal* i)
{
    auto t = coerceTypes(i->op1(), i->op2());
    auto op1 = cg()->llvmValue(i->op1(), t);
    auto op2 = cg()->llvmValue(i->op2(), t);

    auto result = builder()->CreateICmpEQ(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Incr* i)
{
    auto width = ast::rtti::checkedCast<type::Integer>(i->target()->type())->width();
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());

    auto result = builder()->CreateAdd(op1, cg()->llvmConstInt(1, width));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::IncrBy* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateAdd(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Decr* i)
{
    auto width = ast::rtti::checkedCast<type::Integer>(i->target()->type())->width();
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());

    auto result = builder()->CreateSub(op1, cg()->llvmConstInt(1, width));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::DecrBy* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateSub(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Add* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateAdd(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::And* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateAnd(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::AsInterval* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto result = builder()->CreateZExt(op1, cg()->llvmTypeInt(64));
    result = builder()->CreateMul(result, cg()->llvmConstInt(1000000000, 64));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::AsTime* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto result = builder()->CreateZExt(op1, cg()->llvmTypeInt(64));
    result = builder()->CreateMul(result, cg()->llvmConstInt(1000000000, 64));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::AsUDouble* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto result = builder()->CreateUIToFP(op1, cg()->llvmType(i->target()->type()));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::AsSDouble* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto result = builder()->CreateSIToFP(op1, cg()->llvmType(i->target()->type()));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Ashr* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateAShr(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Div* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto ok = cg()->newBuilder("ok");
    auto excpt = cg()->newBuilder("div_by_zero");

    auto width = ast::rtti::checkedCast<type::Integer>(i->target()->type())->width();
    auto is_zero = builder()->CreateICmpNE(op2, cg()->llvmConstInt(0, width));
    cg()->llvmCreateCondBr(is_zero, ok, excpt);

    cg()->pushBuilder(excpt);
    cg()->llvmRaiseException("Hilti::DivisionByZero", i->op2());
    cg()->popBuilder();

    cg()->pushBuilder(ok);
    auto result = builder()->CreateSDiv(op1, op2);
    cg()->llvmStore(i, result);

    // Leave builder on stack.
}

void StatementBuilder::visit(statement::instruction::integer::Eq* i)
{
    auto t = coerceTypes(i->op1(), i->op2());
    auto op1 = cg()->llvmValue(i->op1(), t);
    auto op2 = cg()->llvmValue(i->op2(), t);

    auto result = builder()->CreateICmpEQ(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Mask* i)
{
    auto t = i->target()->type();

    auto op1 = cg()->llvmValue(i->op1(), t);
    auto op2 = cg()->llvmValue(i->op2(), t);
    auto op3 = cg()->llvmValue(i->op3(), t);

    auto result = cg()->llvmExtractBits(op1, op2, op3);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Mod* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto ok = cg()->newBuilder("ok");
    auto excpt = cg()->newBuilder("div_by_zero");

    auto width = ast::rtti::checkedCast<type::Integer>(i->op1()->type())->width();
    auto is_zero = builder()->CreateICmpNE(op2, cg()->llvmConstInt(0, width));
    cg()->llvmCreateCondBr(is_zero, ok, excpt);

    cg()->pushBuilder(excpt);
    cg()->llvmRaiseException("Hilti::DivisionByZero", i->op2());
    cg()->popBuilder();

    cg()->pushBuilder(ok);
    auto result = builder()->CreateSRem(op1, op2);
    cg()->llvmStore(i, result);

    // Leave builder on stack.
}

void StatementBuilder::visit(statement::instruction::integer::Mul* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateMul(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Or* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateOr(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Pow* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    op1 = builder()->CreateZExt(op1, cg()->llvmTypeInt(64));
    op2 = builder()->CreateZExt(op2, cg()->llvmTypeInt(64));

    CodeGen::expr_list args;
    args.push_back(builder::codegen::create(builder::integer::type(64), op1));
    args.push_back(builder::codegen::create(builder::integer::type(64), op2));

    auto pow = cg()->llvmCall("hlt::int_pow", args);

    auto width = ast::rtti::checkedCast<type::Integer>(i->target()->type())->width();
    auto result = builder()->CreateTrunc(pow, cg()->llvmTypeInt(width));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::SExt* i)
{
    auto ty_target = ast::rtti::checkedCast<type::Integer>(i->target()->type());
    auto ty_op1 = ast::rtti::checkedCast<type::Integer>(i->op1()->type());

    auto op1 = cg()->llvmValue(i->op1());

    assert(ty_target->width() >= ty_op1->width());

    llvm::Value* result = 0;

    if ( ty_target->width() > ty_op1->width() )
        result = builder()->CreateSExt(op1, cg()->llvmTypeInt(ty_target->width()));
    else
        result = op1;

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Sgt* i)
{
    auto t = coerceTypes(i->op1(), i->op2());
    auto op1 = cg()->llvmValue(i->op1(), t);
    auto op2 = cg()->llvmValue(i->op2(), t);

    auto result = builder()->CreateICmpSGT(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Shl* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateShl(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Shr* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateLShr(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Sleq* i)
{
    auto t = coerceTypes(i->op1(), i->op2());
    auto op1 = cg()->llvmValue(i->op1(), t);
    auto op2 = cg()->llvmValue(i->op2(), t);

    auto result = builder()->CreateICmpSLE(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Uleq* i)
{
    auto t = coerceTypes(i->op1(), i->op2());
    auto op1 = cg()->llvmValue(i->op1(), t);
    auto op2 = cg()->llvmValue(i->op2(), t);

    auto result = builder()->CreateICmpULE(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Ugeq* i)
{
    auto t = coerceTypes(i->op1(), i->op2());
    auto op1 = cg()->llvmValue(i->op1(), t);
    auto op2 = cg()->llvmValue(i->op2(), t);

    auto result = builder()->CreateICmpUGE(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Slt* i)
{
    auto t = coerceTypes(i->op1(), i->op2());
    auto op1 = cg()->llvmValue(i->op1(), t);
    auto op2 = cg()->llvmValue(i->op2(), t);

    auto result = builder()->CreateICmpSLT(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Sub* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateSub(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Trunc* i)
{
    auto ty_target = ast::rtti::checkedCast<type::Integer>(i->target()->type());
    auto ty_op1 = ast::rtti::checkedCast<type::Integer>(i->op1()->type());

    auto op1 = cg()->llvmValue(i->op1());

    assert(ty_target->width() <= ty_op1->width());

    llvm::Value* result = 0;

    if ( ty_target->width() < ty_op1->width() )
        result = builder()->CreateTrunc(op1, cg()->llvmTypeInt(ty_target->width()));
    else
        result = op1;

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Sgeq* i)
{
    auto t = coerceTypes(i->op1(), i->op2());
    auto op1 = cg()->llvmValue(i->op1(), t);
    auto op2 = cg()->llvmValue(i->op2(), t);

    auto result = builder()->CreateICmpSGE(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Ugt* i)
{
    auto t = coerceTypes(i->op1(), i->op2());
    auto op1 = cg()->llvmValue(i->op1(), t);
    auto op2 = cg()->llvmValue(i->op2(), t);

    auto result = builder()->CreateICmpUGT(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Ult* i)
{
    auto t = coerceTypes(i->op1(), i->op2());
    auto op1 = cg()->llvmValue(i->op1(), t);
    auto op2 = cg()->llvmValue(i->op2(), t);

    auto result = builder()->CreateICmpULT(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Xor* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateXor(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::ZExt* i)
{
    auto ty_target = ast::rtti::checkedCast<type::Integer>(i->target()->type());
    auto ty_op1 = ast::rtti::checkedCast<type::Integer>(i->op1()->type());

    auto op1 = cg()->llvmValue(i->op1());

    assert(ty_target->width() >= ty_op1->width());

    llvm::Value* result = 0;

    if ( ty_target->width() > ty_op1->width() )
        result = builder()->CreateZExt(op1, cg()->llvmTypeInt(ty_target->width()));
    else
        result = op1;

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::ToHost* i)
{
    auto width = ast::rtti::checkedCast<type::Integer>(i->op1()->type())->width();
    auto twidth = ast::rtti::checkedCast<type::Integer>(i->target()->type())->width();

    auto op1 = cg()->llvmValue(i->op1());
    op1 = builder()->CreateZExt(op1, cg()->llvmTypeInt(64));

    CodeGen::expr_list args;
    args.push_back(builder::codegen::create(builder::integer::type(64), op1));
    args.push_back(i->op2());
    args.push_back(builder::integer::create(width / 8));

    auto result = cg()->llvmCall("hlt::int_to_host", args);

    result = builder()->CreateTrunc(result, cg()->llvmTypeInt(twidth));
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::FromHost* i)
{
    auto width = ast::rtti::checkedCast<type::Integer>(i->op1()->type())->width();
    auto twidth = ast::rtti::checkedCast<type::Integer>(i->target()->type())->width();

    auto op1 = cg()->llvmValue(i->op1());
    op1 = builder()->CreateZExt(op1, cg()->llvmTypeInt(64));

    CodeGen::expr_list args;
    args.push_back(builder::codegen::create(builder::integer::type(64), op1));
    args.push_back(i->op2());
    args.push_back(builder::integer::create(width / 8));

    auto result = cg()->llvmCall("hlt::int_from_host", args);

    result = builder()->CreateTrunc(result, cg()->llvmTypeInt(twidth));
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::integer::Flip* i)
{
    auto width = ast::rtti::checkedCast<type::Integer>(i->op1()->type())->width();
    auto twidth = ast::rtti::checkedCast<type::Integer>(i->target()->type())->width();

    auto op1 = cg()->llvmValue(i->op1());
    op1 = builder()->CreateZExt(op1, cg()->llvmTypeInt(64));

    CodeGen::expr_list args;
    args.push_back(builder::codegen::create(builder::integer::type(64), op1));
    args.push_back(builder::integer::create(width / 8));

    auto result = cg()->llvmCall("hlt::int_flip", args);

    result = builder()->CreateTrunc(result, cg()->llvmTypeInt(twidth));
    cg()->llvmStore(i, result);
}
