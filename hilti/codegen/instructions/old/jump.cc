
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::jump::Jump* i)
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
            cg.llvmTailCall(cg.lookupBlock(self.op1()))

    */
}
