
#include <hilti-intern.h>

#include "../stmt-builder.h"
#include "../type-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::operator_::Begin* i)
{
    assert(false); // Not used.
}

void StatementBuilder::visit(statement::instruction::operator_::End* i)
{
    assert(false); // Not used.
}

void StatementBuilder::visit(statement::instruction::operator_::Incr* i)
{
    assert(false); // Not used.
}

void StatementBuilder::visit(statement::instruction::operator_::Deref* i)
{
    assert(false); // Not used.
}

void StatementBuilder::visit(statement::instruction::operator_::Equal* i)
{
    assert(false); // Not used.
}

void StatementBuilder::visit(statement::instruction::operator_::Unequal* i)
{
    auto eq = cg()->makeLocal("eq", builder::boolean::type());
    cg()->llvmInstruction(eq, instruction::operator_::Equal, i->op1(), i->op2());
    auto result = cg()->builder()->CreateSelect(cg()->llvmValue(eq), cg()->llvmConstInt(0, 1), cg()->llvmConstInt(1, 1));
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::operator_::Assign* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type(), true);
    cg()->llvmStore(i, op1);
}

void StatementBuilder::visit(statement::instruction::operator_::Unpack* i)
{
    auto iters = cg()->llvmValue(i->op1());
    auto begin = cg()->llvmTupleElement(i->op1()->type(), iters, 0, false);
    auto end = cg()->llvmTupleElement(i->op1()->type(), iters, 1, false);
    auto fmt = cg()->llvmValue(i->op2());

    auto tl = ast::as<type::Tuple>(i->target()->type())->typeList();
    auto ty = tl.front();

    llvm::Value* arg = i->op3() ? cg()->llvmValue(i->op3()) : nullptr;
    shared_ptr<Type> arg_type = i->op3() ? i->op3()->type() : nullptr;

    auto unpack_result = cg()->llvmUnpack(ty, begin, end, fmt, arg, arg_type, i->location());

    auto result = cg()->llvmTuple({ unpack_result.first, unpack_result.second });
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::operator_::Clear* i)
{
    cg()->llvmStore(i->op1(), cg()->typeInfo(i->op1()->type())->init_val);
}
