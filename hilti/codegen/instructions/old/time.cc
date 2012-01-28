
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::time::Add* i)
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
        result = cg.builder().add(op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::time::AsDouble* i)
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
        val = cg.builder().uitofp(op1, llvm.core.Type.double())
        val = cg.builder().fdiv(val, llvm.core.Constant.real(llvm.core.Type.double(), "1e9"))
        cg.llvmStoreInTarget(self, val)

*/
}

void StatementBuilder::visit(statement::instruction::time::AsInt* i)
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
        result = cg.builder().udiv(op1, cg.llvmConstInt(1000000000, 64))
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::time::Eq* i)
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
        result = cg.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::time::Gt* i)
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
        result = cg.builder().icmp(llvm.core.IPRED_SGT, op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::time::Lt* i)
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
        result = cg.builder().icmp(llvm.core.IPRED_SLT, op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::time::Nsecs* i)
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
        nsecs = cg.llvmCallC("hlt::time_nsecs", [self.op1()])
        cg.llvmStoreInTarget(self, nsecs)

*/
}

void StatementBuilder::visit(statement::instruction::time::Sub* i)
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
        result = cg.builder().sub(op1, op2)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::time::Wall* i)
{

/*
    auto result = builder()->

    cg()->llvmStore(i, result);
*/

/*
    CodeGen::expr_list args;
    auto result = cg()->llvmCall("hlt::X", args);

    cg()->llvmStore(i, result);
*/


/*
    def _codegen(self, cg):
        wall = cg.llvmCallC("hlt::time_wall", [])
        cg.llvmStoreInTarget(self, wall)

*/
}

