
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::double_::Equal* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());
    auto result = cg()->builder()->CreateFCmpOEQ(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::Add* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateFAdd(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::AsInterval* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto result = builder()->CreateFMul(op1, cg()->llvmConstDouble(1e9));
    result = builder()->CreateFPToUI(result, cg()->llvmTypeInt(64));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::AsTime* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto result = builder()->CreateFMul(op1, cg()->llvmConstDouble(1e9));
    result = builder()->CreateFPToUI(result, cg()->llvmTypeInt(64));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::AsSInt* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto result = builder()->CreateFPToSI(op1, cg()->llvmType(i->target()->type()));
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::AsUInt* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto result = builder()->CreateFPToUI(op1, cg()->llvmType(i->target()->type()));
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::Div* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto block_ok = cg()->newBuilder("ok");
    auto block_excpt = cg()->newBuilder("excpt");

    auto not_zero = builder()->CreateFCmpONE(op2, cg()->llvmConstDouble(0));
    cg()->llvmCreateCondBr(not_zero, block_ok, block_excpt);

    cg()->pushBuilder(block_excpt);
    cg()->llvmRaiseException("Hilti::DivisionByZero", i->op2());
    cg()->popBuilder();

    cg()->pushBuilder(block_ok);
    auto result = builder()->CreateFDiv(op1, op2);
    cg()->llvmStore(i, result);

    // Leave builder on stack.
}

void StatementBuilder::visit(statement::instruction::double_::Eq* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());
    auto result = cg()->builder()->CreateFCmpOEQ(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::Gt* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());
    auto result = cg()->builder()->CreateFCmpOGT(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::Geq* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());
    auto result = cg()->builder()->CreateFCmpOGE(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::Lt* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());
    auto result = cg()->builder()->CreateFCmpOLT(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::Leq* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());
    auto result = cg()->builder()->CreateFCmpOLE(op1, op2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::Mod* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto block_ok = cg()->newBuilder("ok");
    auto block_excpt = cg()->newBuilder("excpt");

    auto not_zero = builder()->CreateFCmpONE(op2, cg()->llvmConstDouble(0));
    cg()->llvmCreateCondBr(not_zero, block_ok, block_excpt);

    cg()->pushBuilder(block_excpt);
    cg()->llvmRaiseException("Hilti::DivisionByZero", i->op2());
    cg()->popBuilder();

    cg()->pushBuilder(block_ok);
    auto result = builder()->CreateFRem(op1, op2);
    cg()->llvmStore(i, result);

    // Leave builder on stack.
}

void StatementBuilder::visit(statement::instruction::double_::Mul* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateFMul(op1, op2);

    cg()->llvmStore(i, result);
}

#if 0
void StatementBuilder::visit(statement::instruction::double_::PowInt* i)
{
    llvm::Value* result = 0;

    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2(), shared_ptr<Type>(new type::Integer(32)));
    auto rtype = cg()->llvmType(i->target()->type());

    CodeGen::value_list args;
    args.push_back(op1);
    args.push_back(op2);

    std::vector<llvm::Type *> tys;
    tys.push_back(cg()->llvmTypeDouble());
    
    result = cg()->llvmCallIntrinsic(llvm::Intrinsic::powi, tys, args);

    cg()->llvmStore(i, result);
}
#endif

void StatementBuilder::visit(statement::instruction::double_::PowDouble* i)
{
    llvm::Value* result = 0;

    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    CodeGen::value_list args;
    args.push_back(op1);
    args.push_back(op2);
    result = cg()->llvmCallC("pow", args, false);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::double_::Sub* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto result = builder()->CreateFSub(op1, op2);

    cg()->llvmStore(i, result);
}
