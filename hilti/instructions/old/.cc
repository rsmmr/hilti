
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::::NextCallable* i)
{

/*

    auto result = builder()->

    cg()->llvmStore(i, result);

*/

/*
    CodeGen::expr_list args;
    auto result = cg()->llvmCall("hlt::X", args)

    cg()->llvmStore(i, result);

*/


/*
    def _codegen(self, cg):
        ctx = cg.llvmCurrentExecutionContextPtr()
        c = cg.llvmCallCInternal("__hlt_callable_next", [ctx, cg.llvmFrameExceptionAddr()])
        cg.llvmStoreInTarget(self, c)


*/
}

void StatementBuilder::visit(statement::instruction::::PopHandler* i)
{

/*


*/

/*
    CodeGen::expr_list args;
    cg()->llvmCall("hlt::X", args)

*/


/*
    def _codegen(self, cg):
        cg.currentFunction().popExceptionHandler()


*/
}

void StatementBuilder::visit(statement::instruction::::PushHandler* i)
{

/*

    auto op1 = cg()->llvmValue(i->op1(), trueX);
    auto op2 = cg()->llvmValue(i->op2(), trueX);


*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    cg()->llvmCall("hlt::X", args)

*/


/*
    def _codegen(self, cg):
        handler = cg.lookupBlock(self.op1())
        excpt = self.op2().value() if self.op2() else type.Exception.root()
        cg.currentFunction().pushExceptionHandler(excpt, handler)


*/
}

