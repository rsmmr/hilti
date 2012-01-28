
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::thread::GetContext* i)
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
        context = cg.currentFunction().scope().lookup("context").type()

        tcontext = cg.llvmExecutionContextThreadContext()
        tcontext = cg.builder().bitcast(tcontext, cg.llvmType(context))
        cg.llvmStoreInTarget(self, tcontext)

*/
}

void StatementBuilder::visit(statement::instruction::thread::ThreadID* i)
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
        cg.llvmStoreInTarget(self, cg.llvmExecutionContextVThreadID())

*/
}

void StatementBuilder::visit(statement::instruction::thread::ThreadSchedule* i)
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
        func = cg.lookupFunction(self.op1())

        args = self.op2()
        cont = cg.llvmMakeCallable(func, args, thread_mgr=True)

        mgr = cg.llvmGlobalThreadMgrPtr()
        ctx = cg.llvmCurrentExecutionContextPtr()

        null_tcontext = llvm.core.Constant.null(cg.llvmTypeGenericPointer())

        if self.op3():
            vid = self.op3().coerceTo(cg, type.Integer(64))
            vid = cg.llvmOp(vid)
            cg.llvmCallCInternal("__hlt_thread_mgr_schedule", [mgr, vid, cont, cg.llvmFrameExceptionAddr(), ctx])
            cg.llvmExceptionTest()

        else:
            tcontext = cg.llvmExecutionContextThreadContext()

            # Ensure that we have a context set.
            block_ok = cg.llvmNewBlock("ok")
            block_exc = cg.llvmNewBlock("exc")

            isnull = cg.builder().icmp(llvm.core.IPRED_NE, tcontext, null_tcontext)
            cg.builder().cbranch(isnull, block_ok, block_exc)

            cg.pushBuilder(block_exc)
            cg.llvmRaiseExceptionByName("hlt_exception_no_thread_context", self.location())
            cg.popBuilder()

            cg.pushBuilder(block_ok)

            # Do the Promotion.
            src_scope = cg.currentFunction().threadScope().type()
            src_context = cg.currentFunction().scope().lookup("context").type()
            dst_scope = func.threadScope().type()
            dst_context = func.scope().lookup("context").type()

            tcontext = _promoteContext(cg, tcontext, src_scope, src_context, dst_scope, dst_context)

            ti = cg.builder().bitcast(cg.llvmTypeInfoPtr(dst_context), cg.llvmTypeTypeInfoPtr())
            tcontext = cg.builder().bitcast(tcontext, cg.llvmTypeGenericPointer())
            cg.llvmCallCInternal("__hlt_thread_mgr_schedule_tcontext", [mgr, ti, tcontext, cont, cg.llvmFrameExceptionAddr(), ctx])
            cg.llvmExceptionTest()

*/
}

void StatementBuilder::visit(statement::instruction::thread::SetContext* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    cg()->llvmCall("hlt::X", args);
*/


/*
    def _codegen(self, cg):
        context = cg.currentFunction().scope().lookup("context").type()
        op1 = self.op1().coerceTo(cg, type.Reference(context))
        op1 = cg.llvmOp(op1)
        cg.llvmExecutionContextSetThreadContext(op1)

*/
}

