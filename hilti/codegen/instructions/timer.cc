
#include <hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::timer::New* i)
{
    CodeGen::expr_list params;
    prepareCall(i->op2(), i->op3(), &params);

    auto func = cg()->llvmValue(i->op2(), false);
    auto ftype = ast::as<type::Function>(i->op2()->type());
    auto callable = cg()->llvmCallableBind(func, ftype, params);

    CodeGen::value_list args = { callable };
    auto timer = cg()->llvmCallC("__hlt_timer_new_function", args, true);
    cg()->llvmDtor(callable, builder::reference::type(builder::callable::typeAny()), false, "timer.new");

    cg()->llvmStore(i->target(), timer);

}

void StatementBuilder::visit(statement::instruction::timer::Cancel* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    cg()->llvmCall("hlt::timer_cancel", args);
}

void StatementBuilder::visit(statement::instruction::timer::Update* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::timer_update", args);
}

