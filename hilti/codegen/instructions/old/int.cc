
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::int ::Add* i)
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
            t = self.target().type()
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().add(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::And* i)
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
            t = self.target().type()
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().and_(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::AsDouble* i)
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
            result = cg.builder().zext(op1, llvm.core.Type.int(64))
            result = cg.builder().mul(result, cg.llvmConstInt(1000000000, 64))
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::SInt* i)
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
            result = cg.builder().sitofp(op1, cg.llvmType(self.target().type()))
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::AsTime* i)
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
            result = cg.builder().zext(op1, llvm.core.Type.int(64))
            result = cg.builder().mul(result, cg.llvmConstInt(1000000000, 64))
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::UInt* i)
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
            result = cg.builder().uitofp(op1, cg.llvmType(self.target().type()))
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Ashr* i)
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
            t = self.target().type()
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().ashr(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Div* i)
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
            t = self.target().type()
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)

            block_ok = cg.llvmNewBlock("ok")
            block_exc = cg.llvmNewBlock("exc")

            iszero = cg.builder().icmp(llvm.core.IPRED_NE, op2, cg.llvmConstInt(0, op2.type.width))
            cg.builder().cbranch(iszero, block_ok, block_exc)

            cg.pushBuilder(block_exc)
            cg.llvmRaiseExceptionByName("hlt_exception_division_by_zero", self.location())
            cg.popBuilder()

            cg.pushBuilder(block_ok)
            result = cg.builder().sdiv(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Eq* i)
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
            t = operand.coerceTypes(self.op1(), self.op2())
            assert t
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Xor* i)
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
            t = self.target().type()
            width = t.width()
            op1 = cg.llvmOp(self.op1(), t)

            low = cg.llvmConstInt(self.op2().value().value(), width)
            high = cg.llvmConstInt(self.op3().value().value(), width)

            result = _extractBits(cg, op1, low, high)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Mod* i)
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
            t = self.target().type()
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)

            block_ok = cg.llvmNewBlock("ok")
            block_exc = cg.llvmNewBlock("exc")

            iszero = cg.builder().icmp(llvm.core.IPRED_NE, op2, cg.llvmConstInt(0,
       self.op2().type().width()))
            cg.builder().cbranch(iszero, block_ok, block_exc)

            cg.pushBuilder(block_exc)
            cg.llvmRaiseExceptionByName("hlt_exception_division_by_zero", self.location())
            cg.popBuilder()

            cg.pushBuilder(block_ok)
            result = cg.builder().srem(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Mul* i)
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
            t = self.target().type()
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().mul(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Or* i)
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
            t = self.target().type()
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().or_(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Pow* i)
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
            op1 = cg.builder().zext(op1, llvm.core.Type.int(64))
            op2 = cg.builder().zext(op2, llvm.core.Type.int(64))
            op1 = (op1, type.Integer(64))
            op2 = (op2, type.Integer(64))
            result = cg.llvmCallC("hlt::int_pow", [op1, op2])
            result = cg.builder().trunc(result, llvm.core.Type.int(self.target().type().width()))
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::SExt* i)
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
            width = self.target().type().width()
            assert width >= self.op1().type().width()

            op1 = cg.llvmOp(self.op1())

            result = cg.builder().sext(op1, cg.llvmType(type.Integer(width)))
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Sqeq* i)
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
            t = operand.coerceTypes(self.op1(), self.op2())
            assert t
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().icmp(llvm.core.IPRED_SGE, op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Sgt* i)
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
            t = operand.coerceTypes(self.op1(), self.op2())
            assert t
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().icmp(llvm.core.IPRED_SGT, op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Shl* i)
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
            t = self.target().type()
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().shl(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Shr* i)
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
            t = self.target().type()
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().lshr(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Sleq* i)
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
            t = operand.coerceTypes(self.op1(), self.op2())
            assert t
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().icmp(llvm.core.IPRED_SLE, op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Slt* i)
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
            t = operand.coerceTypes(self.op1(), self.op2())
            assert t
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().icmp(llvm.core.IPRED_SLT, op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Sub* i)
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
            t = self.target().type()
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().sub(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Trunc* i)
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
            width = self.target().type().width()
            assert width <= self.op1().type().width()

            op1 = cg.llvmOp(self.op1())

            result = cg.builder().trunc(op1, cg.llvmType(type.Integer(width)))
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Sqeq* i)
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
            t = operand.coerceTypes(self.op1(), self.op2())
            assert t
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().icmp(llvm.core.IPRED_UGE, op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Ugt* i)
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
            t = operand.coerceTypes(self.op1(), self.op2())
            assert t
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().icmp(llvm.core.IPRED_UGT, op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Sleq* i)
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
            t = operand.coerceTypes(self.op1(), self.op2())
            assert t
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().icmp(llvm.core.IPRED_ULE, op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Ult* i)
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
            t = operand.coerceTypes(self.op1(), self.op2())
            assert t
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().icmp(llvm.core.IPRED_ULT, op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::Xor* i)
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
            t = self.target().type()
            op1 = cg.llvmOp(self.op1(), t)
            op2 = cg.llvmOp(self.op2(), t)
            result = cg.builder().xor(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::int ::ZExt* i)
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
            width = self.target().type().width()
            assert width >= self.op1().type().width()

            op1 = cg.llvmOp(self.op1())

            result = cg.builder().zext(op1, cg.llvmType(type.Integer(width)))
            cg.llvmStoreInTarget(self, result)

    */
}
