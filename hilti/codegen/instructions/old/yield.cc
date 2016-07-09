
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::yield::Yield* i)
{
    /*
    */

    /*
        CodeGen::expr_list args;
        cg()->llvmCall("hlt::X", args);
    */


    /*
        def _codegen(self, cg):
            next = cg.currentBlock().next()
            next = cg.currentFunction().lookupBlock(next)
            assert next
            succ = cg.llvmFunctionForBlock(next)
            cg.llvmYield(succ)

    */
}

void StatementBuilder::visit(statement::instruction::yield::YieldUntil* i)
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
            next = cg.currentBlock().next()
            next = cg.currentFunction().lookupBlock(next)
            assert next
            succ = cg.llvmFunctionForBlock(next)

            op1 = cg.llvmOp(self.op1())
            blockable = cg.llvmBlockable(self.op1().type(), op1)
            cg.llvmYield(succ, blockable=blockable)

    */
}
