
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::operator_::Assign* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type(), true);
    cg()->llvmStore(i, op1);
}

