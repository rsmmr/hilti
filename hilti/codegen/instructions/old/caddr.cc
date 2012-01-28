
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::caddr::Function* i)
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

        fid = self.op1().value()
        func = cg.lookupFunction(fid)
        builder = cg.builder()

        if func.callingConvention() == function.CallingConvention.HILTI:
            (hltmain, main, hltresume, resume) = cg.llvmCStubs(func)
            main = builder.bitcast(main, cg.llvmTypeGenericPointer())
            resume = builder.bitcast(resume, cg.llvmTypeGenericPointer())
        elif func.callingConvention() == function.CallingConvention.C_HILTI:
            func = cg.llvmFunction(func)
            main = builder.bitcast(func, cg.llvmTypeGenericPointer())
            resume = llvm.core.Constant.null(cg.llvmTypeGenericPointer())
        else:
            util.internal_error("caddr.Function not supported for non-HILTI/HILTI-C functions yet")

        struct = llvm.core.Constant.struct([main, resume])
        cg.llvmStoreInTarget(self, struct)

*/
}

