
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::return::ReturnResult* i)
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
        op1 = cg.llvmOp(self.op1())

        if cg.currentFunction().callingConvention() == function.CallingConvention.HILTI:
            rtype = cg.currentFunction().type().resultType()
            succ = cg.llvmFrameNormalSucc()
            frame = cg.llvmFrameNormalFrame()

            addr = cg._llvmAddrInBasicFrame(frame.fptr(), 0, 2)
            cg.llvmTailCall(succ, frame=frame, addl_args=[(op1, rtype)], clear=True)
        else:
            cg.builder().ret(op1)

*/
}

void StatementBuilder::visit(statement::instruction::return::ReturnVoid* i)
{

/*
*/

/*
    CodeGen::expr_list args;
    cg()->llvmCall("hlt::X", args);
*/


/*
    def _codegen(self, cg):
        if cg.currentFunction().callingConvention() == function.CallingConvention.HILTI:
            succ = cg.llvmFrameNormalSucc()
            frame = cg.llvmFrameNormalFrame()
            cg.llvmTailCall(succ, frame=frame, clear=True)
        else:
            cg.builder().ret_void()

*/
}

