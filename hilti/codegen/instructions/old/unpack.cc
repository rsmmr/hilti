
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::unpack::Unpack* i)
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
            op1 = cg.llvmOp(self.op1())

            begin = cg.llvmExtractValue(op1, 0)
            end = cg.llvmExtractValue(op1, 1)

            t = self.target().type().types()[0]

            (val, iter) = cg.llvmUnpack(t, begin, end, self.op2(), self.op3())

            val = operand.LLVM(val, self.target().type().types()[0])
            iter = operand.LLVM(iter, self.target().type().types()[1])

            tuple = constant.Constant([val, iter], type.Tuple([type.IteratorBytes()] * 2))
            cg.llvmStoreInTarget(self, operand.Constant(tuple))

    */
}
