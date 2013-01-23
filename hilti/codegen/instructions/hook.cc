
#include <hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::hook::DisableGroup* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(builder::boolean::create(0));
    cg()->llvmCall("hlt::hook_group_enable", args);
}

void StatementBuilder::visit(statement::instruction::hook::EnableGroup* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(builder::boolean::create(1));
    cg()->llvmCall("hlt::hook_group_enable", args);
}

void StatementBuilder::visit(statement::instruction::hook::GroupEnabled* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::hook_group_is_enabled", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::hook::Run* i)
{
    auto func = ast::as<expression::Function>(i->op1())->function();
    auto has_result = (! ast::isA<type::Void>(func->type()->result()->type()));
    auto hook = ast::as<Hook>(func);
    assert(hook);

    llvm::Value* result = nullptr;

    if ( has_result )
        result = cg()->llvmAddTmp("hook.rval", cg()->llvmType(i->target()->type()), nullptr, true);

    CodeGen::expr_list params;
    prepareCall(i->op1(), i->op2(), &params, true);
    auto stopped = cg()->llvmRunHook(hook, params, result);

    if ( has_result ) {
        auto cont = cg()->newBuilder("cont");
        auto load_result = cg()->pushBuilder("result");
        auto rtype = func->type()->result()->type();

        cg()->llvmStore(i, builder()->CreateLoad(result));
        cg()->llvmCreateBr(cont);
        cg()->popBuilder();
        cg()->llvmCreateCondBr(stopped, load_result, cont);
        cg()->pushBuilder(cont); // Leave on stack.
    }
}

void StatementBuilder::visit(statement::instruction::hook::Stop* i)
{
    if ( i->op1() ) {
        // Store result in last parameter.
        auto val = cg()->llvmValue(i->op1(), nullptr, true);
        cg()->llvmCreateStore(val, --cg()->function()->arg_end());
    }

    cg()->llvmBuildInstructionCleanup();
    cg()->llvmReturn(0, cg()->llvmConstInt(1, 1)); // Return true.
}

