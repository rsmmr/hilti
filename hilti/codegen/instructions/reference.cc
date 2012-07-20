
#include <hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::reference::AsBool* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    op1 = cg()->builder()->CreateBitCast(op1, cg()->llvmTypePtr());
    auto result = builder()->CreateIsNull(op1);
    cg()->llvmStore(i, result);
}

