
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::hook::DisableGroup* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(builder::boolean::create(0));
    cg()->llvmCall("hlt::hook)_group_enable", args);
}

void StatementBuilder::visit(statement::instruction::hook::EnableGroup* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(builder::boolean::create(1));
    cg()->llvmCall("hlt::hook)_group_enable", args);
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
    // TODO: Not implemented.
    assert(false);
/*
    def _codegen(self, cg):
        # This is canonified away.
        assert False

*/
}

void StatementBuilder::visit(statement::instruction::hook::Stop* i)
{
    // TODO: Not implemented.
    assert(false);
/*
    def _codegen(self, cg):
        # This is canonified away.
        assert False

*/
}

