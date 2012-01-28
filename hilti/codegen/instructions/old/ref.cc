
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::ref::CastBool* i)
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
        op1 = cg.llvmOp(self.op1())
        voidp = cg.builder().bitcast(op1, cg.llvmTypeGenericPointer())
        null = llvm.core.Constant.null(cg.llvmTypeGenericPointer())
        result = cg.builder().icmp(llvm.core.IPRED_NE, voidp, null)
        cg.llvmStoreInTarget(self, result)

*/
}

