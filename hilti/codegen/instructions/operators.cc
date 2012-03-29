
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

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

    auto ty = ast::as<type::Tuple>(i->target())->typeList().begin();

    llvm::Value* arg = i->op2() ? cg()->llvmValue(i->op2()) : nullptr;

    auto unpack_result = cg()->llvmUnpack(*ty, begin, end, fmt, arg, i->location());

    auto result = cg()->llvmTuple(i->target()->type(), { unpack_result.first, unpack_result.second });
    cg()->llvmStore(i, result);
}

