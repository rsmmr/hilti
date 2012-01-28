
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::boolean::And* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateAnd(op1, op2);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::boolean::Not* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());

    auto result = builder()->CreateXor(op1, cg()->llvmConstInt(1, 1));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::boolean::Or* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());

    auto result = builder()->CreateOr(op1, op2);

    cg()->llvmStore(i, result);
}

