
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::double::Add* i)
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
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fadd(op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::double::AsInterval* i)
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
        val = cg.builder().fmul(op1, llvm.core.Constant.real(llvm.core.Type.double(), "1e9"))
        val = cg.builder().fptoui(val, llvm.core.Type.int(64))
        cg.llvmStoreInTarget(self, val)

*/
}

void StatementBuilder::visit(statement::instruction::double::AsUint* i)
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
        result = cg.builder().fptosi(op1, cg.llvmType(self.target().type()))
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::double::AsTime* i)
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
        val = cg.builder().fmul(op1, llvm.core.Constant.real(llvm.core.Type.double(), "1e9"))
        val = cg.builder().fptoui(val, llvm.core.Type.int(64))
        cg.llvmStoreInTarget(self, val)

*/
}

void StatementBuilder::visit(statement::instruction::double::AsUint* i)
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
        result = cg.builder().fptoui(op1, cg.llvmType(self.target().type()))
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::double::Div* i)
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
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())

        block_ok = cg.llvmNewBlock("ok")
        block_exc = cg.llvmNewBlock("exc")

        iszero = cg.builder().fcmp(llvm.core.RPRED_ONE, op2, cg.llvmConstDouble(0))
        cg.builder().cbranch(iszero, block_ok, block_exc)

        cg.pushBuilder(block_exc)
        cg.llvmRaiseExceptionByName("hlt_exception_division_by_zero", self.location())
        cg.popBuilder()

        cg.pushBuilder(block_ok)
        result = cg.builder().fdiv(op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::double::Eq* i)
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
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fcmp(llvm.core.RPRED_OEQ, op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::double::Gt* i)
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
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fcmp(llvm.core.RPRED_OGT, op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::double::Lt* i)
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
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fcmp(llvm.core.RPRED_OLT, op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::double::Mod* i)
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
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())

        block_ok = cg.llvmNewBlock("ok")
        block_exc = cg.llvmNewBlock("exc")

        iszero = cg.builder().fcmp(llvm.core.RPRED_ONE, op2, cg.llvmConstDouble(0))
        cg.builder().cbranch(iszero, block_ok, block_exc)

        cg.pushBuilder(block_exc)
        cg.llvmRaiseExceptionByName("hlt_exception_division_by_zero", self.location())
        cg.popBuilder()

        cg.pushBuilder(block_ok)
        result = cg.builder().frem(op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::double::Mul* i)
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
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fmul(op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::double::Pow* i)
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
        op1 = cg.llvmOp(self.op1())

        if isinstance(self.op2().type(), type.Integer):
            op2 = cg.llvmOp(self.op2(), coerce_to=type.Integer(32))
            rtype = cg.llvmType(self.target().type())
            result = cg.llvmCallIntrinsic(llvm.core.INTR_POWI, [rtype], [op1, op2])
            cg.llvmStoreInTarget(self, result)
        else:
            op2 = cg.llvmOp(self.op2())
            result = cg.llvmCallCInternal("pow", [op1, op2])
            cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::double::Sub* i)
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
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())
        result = cg.builder().fsub(op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

