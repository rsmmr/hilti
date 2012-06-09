
#include <hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::callable::New* i)
{
    CodeGen::expr_list params;
    prepareCall(i->op2(), i->op3(), &params);

    auto func = cg()->llvmValue(i->op2(), false);
    auto ftype = ast::as<type::Function>(i->op2()->type());
    auto result = cg()->llvmCallableBind(func, ftype, params);

    cg()->llvmStore(i->target(), result);
}

