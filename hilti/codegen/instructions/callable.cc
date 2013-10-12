
#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::callable::NewFunction* i)
{
    CodeGen::expr_list params;
    prepareCall(i->op2(), i->op3(), &params, false);

    auto func = cg()->llvmValue(i->op2());
    auto ftype = ast::as<type::Function>(i->op2()->type());
    auto result = cg()->llvmCallableBind(func, ftype, params);

    cg()->llvmStore(i->target(), result);
}

void StatementBuilder::visit(statement::instruction::callable::NewHook* i)
{
    auto func = ast::checkedCast<expression::Function>(i->op2())->function();
    auto has_result = (! ast::isA<type::Void>(func->type()->result()->type()));
    auto hook = ast::checkedCast<Hook>(func);
    assert(hook);

    CodeGen::expr_list params;
    prepareCall(i->op2(), i->op3(), &params, false);

    auto llvm_func = cg()->llvmFunctionHookRun(hook);
    auto ftype = ast::as<type::Hook>(i->op2()->type());
    auto result = cg()->llvmCallableBind(hook, params);

    cg()->llvmStore(i->target(), result);
}

