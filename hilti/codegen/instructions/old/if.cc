
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::if::IfElse* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
    auto op2 = cg()->llvmValue(i->op2(), X);
    auto op3 = cg()->llvmValue(i->op3(), X);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());
    cg()->llvmCall("hlt::X", args);
*/


/*
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1(), type.Bool())
        true = cg.lookupBlock(self.op2())
        false = cg.lookupBlock(self.op3())

        block_true = cg.llvmNewBlock("true")
        block_false = cg.llvmNewBlock("false")

        cg.pushBuilder(block_true)

        cg.llvmTailCall(true)
        cg.popBuilder()

        cg.pushBuilder(block_false)
        cg.llvmTailCall(false)
        cg.popBuilder()

        cg.builder().cbranch(op1, block_true, block_false)

*/
}

