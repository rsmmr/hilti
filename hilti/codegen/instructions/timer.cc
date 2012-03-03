
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::timer::New* i)
{
    assert(false);
    // Not ye implemented.

/*
        func = cg.lookupFunction(self.op2())
        args = self.op3()
        cont = cg.llvmMakeCallable(func, args)

        result = cg.llvmCallCInternal("__hlt_timer_new_function", [cont, cg.llvmFrameExceptionAddr(), cg.llvmCurrentExecutionContextPtr()])
        cg.llvmExceptionTest()
        cg.llvmStoreInTarget(self, result)
*/
}

void StatementBuilder::visit(statement::instruction::timer::Cancel* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    cg()->llvmCall("hlt::timer_cancel", args);
}

void StatementBuilder::visit(statement::instruction::timer::Update* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::timer_update", args);
}

