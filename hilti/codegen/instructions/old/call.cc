
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::call::Call* i)
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
        # Can't be called because canonicalization will transform these into
        # other calls.
        assert False

*/
}

void StatementBuilder::visit(statement::instruction::call::CallC* i)
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
        func = cg.lookupFunction(self.op1())
        result = cg.llvmCallC(func, self.op2())
        if self.target():
            cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::call::CallTailResult* i)
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
        args = self.op2()

        # Build a helper function that receives the result.
        result_name = cg.nameFunctionForBlock(cg.currentBlock()) + "_result"

        if isinstance(self.op1().value(), id.Function):
            func = cg.lookupFunction(self.op1())
            result_type = cg.llvmType(func.type().resultType())

            succ = cg.lookupBlock(self.op3())
            llvm_succ = cg.llvmFunctionForBlock(succ)
        else:
            result_type = cg.llvmType(self.op1().type().refType().resultType())

            succ = cg.lookupBlock(self.op2())
            llvm_succ = cg.llvmFunctionForBlock(succ)

        result_func = cg.llvmNewHILTIFunction(cg.currentFunction(), result_name, [("__result", result_type)])
        result_block = result_func.append_basic_block("")

        cg.pushBuilder(result_block)
        cg.llvmStoreLocalVar(self.target().value(), result_func.args[3], frame=result_func.args[0])
        fdesc = cg.makeFrameDescriptor(result_func.args[0], result_func.args[1])

        cg.llvmTailCall(llvm_succ, frame=fdesc, ctx=result_func.args[2]) # XXX
        cg.builder().ret_void()
        cg.popBuilder()

        if isinstance(self.op1().value(), id.Function):
            func = cg.lookupFunction(self.op1())
            frame = cg.llvmMakeFunctionFrame(func, args, result_func)
            cg.llvmTailCall(func, frame=frame)
        else:
            ptr = cg.llvmOp(self.op1())
            cg.llvmCallCallable(cg.builder().load(ptr), result_func)

*/
}

void StatementBuilder::visit(statement::instruction::call::CallTailVoid* i)
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
        if isinstance(self.op1().value(), id.Function):
            func = cg.lookupFunction(self.op1())
            args = self.op2()

            succ = cg.lookupBlock(self.op3())
            llvm_succ = cg.llvmFunctionForBlock(succ)

            frame = cg.llvmMakeFunctionFrame(func, args, llvm_succ)
            cg.llvmTailCall(func, frame=frame)

        else:
            succ = cg.lookupBlock(self.op2())
            llvm_succ = cg.llvmFunctionForBlock(succ)

            ptr = cg.llvmOp(self.op1())
            cg.llvmCallCallable(cg.builder().load(ptr), llvm_succ)

*/
}

