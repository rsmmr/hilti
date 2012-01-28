
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::callable::Bind* i)
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
        func = self.op2().value().function()
        args = self.op3()
        cont = cg.llvmMakeCallable(func, args)

        ptr = cg.llvmOp(self.op1())
        cg.builder().store(cont, ptr)

*/
}

