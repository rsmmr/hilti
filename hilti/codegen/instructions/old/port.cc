
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::port::Protocol* i)
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
        op1 = cg.llvmOp(self.op1())

        builder = cg.builder()
        v = cg.llvmExtractValue(op1, 1)

        cmp = builder.icmp(llvm.core.IPRED_EQ, v, cg.llvmConstInt(Port._protos["tcp"], 8))

        tcp = cg.llvmEnumLabel("Hilti::Protocol::TCP")
        udp = cg.llvmEnumLabel("Hilti::Protocol::UDP")
        result = cg.builder().select(cmp, tcp, udp)

        cg.llvmStoreInTarget(self, result)

*/
}

