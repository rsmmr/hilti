
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::bitset::Clear* i)
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
            op2 = cg.builder().xor(op2, cg.llvmConstInt(-1, 64))
            result = cg.builder().and_(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::bitset::Has* i)
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
            tmp = cg.builder().and_(op1, op2)
            result = cg.builder().icmp(llvm.core.IPRED_EQ, tmp, op2)
            cg.llvmStoreInTarget(self, result)

    */
}

void StatementBuilder::visit(statement::instruction::bitset::Set* i)
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
            result = cg.builder().or_(op1, op2)
            cg.llvmStoreInTarget(self, result)

    */
}
