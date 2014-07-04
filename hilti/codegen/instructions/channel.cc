
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::channel::New* i)
{
   auto etype = ast::as<type::Channel>(typedType(i->op1()))->argType();
   auto op1 = builder::type::create(etype);

   auto op2 = i->op2();

   if ( ! op2 )
       op2 = builder::integer::create(0);

    CodeGen::expr_list args;
    args.push_back(op1);
    args.push_back(op2);
    auto result = cg()->llvmCall("hlt::channel_new", args);
    cg()->llvmStore(i, result);
}

static llvm::Value* _readTry(CodeGen* cg, statement::Instruction* i)
{
    CodeGen::expr_list args = { i->op1() };
    return cg->llvmCall("hlt::channel_read_try", args, false, false);
}

static void _readFinish(CodeGen* cg, statement::Instruction* i, llvm::Value* result)
{
    auto rtype = ast::checkedCast<type::Reference>(i->op1()->type());
    auto etype = ast::checkedCast<type::Channel>(rtype->argType())->argType();
    auto casted = cg->builder()->CreateBitCast(result, cg->llvmTypePtr(cg->llvmType(etype)));
    result = cg->builder()->CreateLoad(casted);
    cg->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::channel::Read* i)
{
    cg()->llvmBlockingInstruction(i, _readTry, _readFinish);
}

void StatementBuilder::visit(statement::instruction::channel::ReadTry* i)
{
    auto val = _readTry(cg(), i);
    cg()->llvmCheckException();

    _readFinish(cg(), i, val);
}

void StatementBuilder::visit(statement::instruction::channel::Size* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::channel_size", args);

    cg()->llvmStore(i, result);
}

static llvm::Value* _writeTry(CodeGen* cg, statement::Instruction* i)
{
    auto rtype = ast::checkedCast<type::Reference>(i->op1()->type());
    auto etype = ast::checkedCast<type::Channel>(rtype->argType())->argType();

    CodeGen::expr_list args = { i->op1(), i->op2()->coerceTo(etype) };
    return cg->llvmCall("hlt::channel_write_try", args, false, false);
}

static void _writeFinish(CodeGen* cg, statement::Instruction* i, llvm::Value* result)
{
    // Nothing to do.
}

void StatementBuilder::visit(statement::instruction::channel::Write* i)
{
    cg()->llvmBlockingInstruction(i, _writeTry, _writeFinish);
}

void StatementBuilder::visit(statement::instruction::channel::WriteTry* i)
{
    auto val = _writeTry(cg(), i);
    cg()->llvmCheckException();

    _writeFinish(cg(), i, val);
}

