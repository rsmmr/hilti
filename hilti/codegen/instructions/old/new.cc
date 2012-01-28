
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::new::New* i)
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
        ops = _findOverloadedOperator(self)
        assert ops
        self._callOps(ops, "_codegen", cg)

        for op in ops:
            self._llvm_target = op._llvm_target

*/
}

