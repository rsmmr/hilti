
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::integer::Add* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());
    auto result = builder()->CreateAdd(op1, op2);
    cg()->llvmStore(i->target(), result);
}

void StatementBuilder::visit(statement::instruction::integer::Sub* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());
    auto result = builder()->CreateSub(op1, op2);
    cg()->llvmStore(i, result);
}
