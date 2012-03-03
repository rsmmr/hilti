
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::callable::New* i)
{
    assert(false);
/*
        ptr = cg.llvmMalloc(cg.llvmTypeContinuationPtr(), tag="new <callable>", location=self.location())
        null = llvm.core.Constant.null(cg.llvmTypeContinuationPtr())
        cg.builder().store(null, ptr)
        cg.llvmStoreInTarget(self, ptr)
*/
}

void StatementBuilder::visit(statement::instruction::callable::Bind* i)
{
    assert(false);

    /// TODO: We should get rid of callable.new/bind and just do "bind"
    /// operator in flow.cc, just like "call".

/*
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());
    auto op3 = cg()->llvmValue(i->op3());

    cg()->llvmBindCall(op2, op3);
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

