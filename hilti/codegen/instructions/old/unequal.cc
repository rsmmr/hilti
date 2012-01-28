
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::unequal::Unequal* i)
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
        eq = Equal(op1=self.op1(), op2=self.op2(), target=self.target())
        eq.disableTarget()
        eq.codegen(cg)
        negated = cg.builder().xor(eq.llvmTarget(), cg.llvmConstInt(1, 1))
        cg.llvmStoreInTarget(self, negated)

*/
}

