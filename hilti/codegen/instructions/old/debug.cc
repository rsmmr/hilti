
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::debug::Assert* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
    auto op2 = cg()->llvmValue(i->op2(), X);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::X", args);
*/


/*
    def _codegen(self, cg):
        if cg.debugLevel() == 0:
            return

        op1 = cg.llvmOp(self.op1())
        block_true = cg.llvmNewBlock("true")
        block_false = cg.llvmNewBlock("false")

        cg.builder().cbranch(op1, block_true, block_false)

        cg.pushBuilder(block_false)
        arg = cg.llvmOp(self.op2()) if self.op2() else string._makeLLVMString(cg, "")
        cg.llvmRaiseExceptionByName("hlt_exception_assertion_error", self.location(), arg)
        cg.popBuilder()

        cg.pushBuilder(block_true)

*/
}

void StatementBuilder::visit(statement::instruction::debug::InternalError* i)
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
        arg = cg.llvmOp(self.op1())
        cg.llvmRaiseExceptionByName("hlt_exception_internal_error", self.location(), arg)

*/
}

void StatementBuilder::visit(statement::instruction::debug::Msg* i)
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
        if cg.debugLevel() == 0:
            return

        cg.llvmCallC("hlt::debug_printf", [self.op1(), self.op2(), self.op3()])

*/
}

void StatementBuilder::visit(statement::instruction::debug::InternalError* i)
{

/*
*/

/*
    CodeGen::expr_list args;
    cg()->llvmCall("hlt::X", args);
*/


/*
    def _codegen(self, cg):
        cg.llvmDebugPopIndent()

*/
}

void StatementBuilder::visit(statement::instruction::debug::PrintFrame* i)
{

/*
*/

/*
    CodeGen::expr_list args;
    cg()->llvmCall("hlt::X", args);
*/


/*
    def _codegen(self, cg):
        cg.llvmDebugPrintPtr("dbg.print-frame: normal succ ", cg.llvmFrameNormalSucc())
        cg.llvmDebugPrintPtr("dbg.print-frame: normal frame", cg.llvmFrameNormalFrame().fptr())
        cg.llvmDebugPrintPtr("dbg.print-frame: excpt succ  ", cg.llvmFrameExcptSucc())
        cg.llvmDebugPrintPtr("dbg.print-frame: excpt frame ", cg.llvmFrameExcptFrame().fptr())
        cg.llvmDebugPrintPtr("dbg.print-frame: excpt ptr",    cg.llvmFrameException())

*/
}

void StatementBuilder::visit(statement::instruction::debug::InternalError* i)
{

/*
*/

/*
    CodeGen::expr_list args;
    cg()->llvmCall("hlt::X", args);
*/


/*
    def _codegen(self, cg):
        cg.llvmDebugPushIndent()

*/
}

