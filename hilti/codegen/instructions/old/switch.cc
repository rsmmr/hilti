
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::switch::Switch* i)
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
        op1 = cg.llvmOp(self.op1())
        optype = self.op1().type()
        default = cg.currentFunction().lookupBlock(self.op2().value().name())
        alts = self.op3().value().value()
        vals = [op.value().value()[0] for op in alts]
        names = [op.value().value()[1].value().name() for op in alts]

        dests = [cg.currentFunction().lookupBlock(name) for name in names]
        blocks = [cg.llvmNewBlock("switch-%s" % name) for name in names]

        for (d, b) in zip(dests, blocks):
            cg.pushBuilder(b)
            cg.llvmTailCall(d)
            cg.popBuilder()

        default_block = cg.llvmNewBlock("switch-default")
        cg.pushBuilder(default_block)
        cg.llvmTailCall(default)
        cg.popBuilder()

        # We optimize for integers here, for which LLVM directly provides a switch
        # statement.
        if isinstance(optype, type.Integer):
            switch = cg.builder().switch(op1, default_block)
            for (val, block) in zip(vals, blocks):
                switch.add_case(cg.llvmOp(val, coerce_to=optype), block)

        else:
            # In all other cases, we build an if-else chain using the type's
            # standard comparision operator.
            tmp = cg.builder().alloca(llvm.core.Type.int(1))
            target = operand.LLVM(tmp, type.Bool())

            for (val, block) in zip(vals, blocks):
                # We build an artifical equal operator; just the types need to match.
                i = operators.Equal(target=target, op1=self.op1(), op2=val)
                i.codegen(cg)
                match = cg.builder().icmp(llvm.core.IPRED_EQ, cg.llvmConstInt(1, 1), cg.builder().load(tmp))

                next_block = cg.llvmNewBlock("switch-chain")
                cg.builder().cbranch(match, block, next_block)
                cg.pushBuilder(next_block)

            cg.builder().branch(default_block)

*/
}

