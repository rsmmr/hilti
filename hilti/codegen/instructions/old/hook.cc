
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::hook::DisableGroup* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    cg()->llvmCall("hlt::X", args);
*/


/*
    def _codegen(self, cg):
        cg.llvmCallC("hlt::hook_group_enable", [self.op1(), operand.Constant(constant.Constant(0, type.Bool()))])

*/
}

void StatementBuilder::visit(statement::instruction::hook::EnableGroup* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    cg()->llvmCall("hlt::X", args);
*/


/*
    def _codegen(self, cg):
        cg.llvmCallC("hlt::hook_group_enable", [self.op1(), operand.Constant(constant.Constant(1, type.Bool()))])

*/
}

void StatementBuilder::visit(statement::instruction::hook::GroupEnabled* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);

    auto result = builder()->

    cg()->llvmStore(i, result);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::X", args);

    cg()->llvmStore(i, result);
*/


/*
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::hook_group_is_enabled", [self.op1()])
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::hook::Run* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
    auto op2 = cg()->llvmValue(i->op2(), X);

    auto result = builder()->

    cg()->llvmStore(i, result);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::X", args);

    cg()->llvmStore(i, result);
*/


/*
    def _codegen(self, cg):
        # This is canonified away.
        assert False

*/
}

void StatementBuilder::visit(statement::instruction::hook::Stop* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    cg()->llvmCall("hlt::X", args);
*/


/*
    def _codegen(self, cg):
        # Canonfied away.
        assert False

*/
}

