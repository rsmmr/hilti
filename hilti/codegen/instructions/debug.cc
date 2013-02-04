
#include <hilti-intern.h>

#include "../stmt-builder.h"
#include "options.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::debug::Assert* i)
{
    if ( cg()->options().debug == 0 )
        return;

    auto op1 = cg()->llvmValue(i->op1());
    auto block_true = cg()->newBuilder("true");
    auto block_false = cg()->newBuilder("false");

    cg()->llvmCreateCondBr(op1, block_true, block_false);

    cg()->pushBuilder(block_false);
    auto arg = i->op2() ? cg()->llvmValue(i->op2()) : cg()->llvmString("");
    cg()->llvmRaiseException("Hilti::AssertionError", i->op1(), arg);
    cg()->popBuilder();

    cg()->pushBuilder(block_true);

    // Leave on stack.
}

void StatementBuilder::visit(statement::instruction::debug::InternalError* i)
{
    cg()->llvmRaiseException("Hilti::InternalError", i->location(), cg()->llvmValue(i->op1()));
}

void StatementBuilder::visit(statement::instruction::debug::Msg* i)
{
    CodeGen::expr_list args = { i->op1(), i->op2(), i->op3() };
    cg()->llvmCall("hlt::debug_printf", args);
}

void StatementBuilder::visit(statement::instruction::debug::PopIndent* i)
{
    cg()->llvmDebugPopIndent();
}

void StatementBuilder::visit(statement::instruction::debug::PushIndent* i)
{
    cg()->llvmDebugPushIndent();
}

