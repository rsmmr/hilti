
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::enum::FromInt* i)
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
        labels = self.target().type().labels().items()

        block_cont = cg.llvmNewBlock("enum-cont")

        block_known = cg.llvmNewBlock("enum-known")
        cg.pushBuilder(block_known)
        cg.builder().branch(block_cont)
        cg.popBuilder()

        block_unknown = cg.llvmNewBlock("enum-unknown")
        cg.pushBuilder(block_unknown)
        cg.builder().branch(block_cont)
        cg.popBuilder()

        op = cg.llvmOp(self.op1())
        switch = cg.builder().switch(op, block_unknown)

        have_vals = set()

        cases = []
        for (label, value) in sorted(labels):
            if value in have_vals:
                # If one value has multiple labels, we just take the first.
                continue

            switch.add_case(cg.llvmConstInt(value, 64), block_known)
            have_vals.add(value)

        cg.pushBuilder(block_cont)

        phi = cg.builder().phi(llvm.core.Type.int(1))
        phi.add_incoming(cg.llvmConstInt(0, 1), block_known)
        phi.add_incoming(cg.llvmConstInt(1, 1), block_unknown)

        result = _makeValLLVM(cg, phi, cg.llvmConstInt(1, 1), op)
        cg.llvmStoreInTarget(self, result)

*/
}

