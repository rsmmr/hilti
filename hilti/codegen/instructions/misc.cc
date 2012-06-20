
#include <hilti-intern.h>

#include "../stmt-builder.h"
#include "autogen/instructions.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::Misc::Select* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());
    auto op3 = cg()->llvmValue(i->op3(), i->target()->type());

    auto result = cg()->builder()->CreateSelect(op1, op2, op3);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::Misc::Nop* i)
{
    // Nothing to do.
}
