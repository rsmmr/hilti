
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::select::Select* i)
{
    /*
        auto op1 = cg()->llvmValue(i->op1(), X);
        auto op2 = cg()->llvmValue(i->op2(), X);
        auto op3 = cg()->llvmValue(i->op3(), X);

        auto result = builder()->

        cg()->llvmStore(i, result);
    */

    /*
        CodeGen::expr_list args;
        args.push_back(i->op1());
        args.push_back(i->op2());
        args.push_back(i->op3());

        auto result = cg()->llvmCall("hlt::X", args);

        cg()->llvmStore(i, result);
    */


    /*
        def _codegen(self, cg):
            op1 = cg.llvmOp(self.op1())
            op2 = cg.llvmOp(self.op2())
            op3 = cg.llvmOp(self.op3())
            select = cg.builder().select(op1, op2, op3)
            cg.llvmStoreInTarget(self, select)

    */
}
