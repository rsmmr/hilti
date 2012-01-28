
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::timer_mgr::Advance* i)
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
        cg.llvmCallC("hlt::timer_mgr_advance", [self.op1(), self.op2()])

*/
}

void StatementBuilder::visit(statement::instruction::timer_mgr::Current* i)
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
        result = cg.llvmCallC("hlt::timer_mgr_current", [self.op1()])
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::timer_mgr::Expire* i)
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
        cg.llvmCallC("hlt::timer_mgr_expire", [self.op1(), self.op2()])

*/
}

void StatementBuilder::visit(statement::instruction::timer_mgr::Schedule* i)
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
        cg.llvmCallC("hlt::timer_mgr_schedule", [self.op1(), self.op2(), self.op3()])

*/
}

