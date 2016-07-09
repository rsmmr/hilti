
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::addr::Family* i)
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
            v = cg.llvmExtractValue(op1, 0)
            cmp1 = builder.icmp(llvm.core.IPRED_EQ, v, cg.llvmConstInt(0, 64))

            v = cg.llvmExtractValue(op1, 1)
            masked = cg.builder().and_(v, cg.llvmConstInt(0xffffffff00000000, 64))
            cmp2 = cg.builder().icmp(llvm.core.IPRED_EQ, masked, cg.llvmConstInt(0, 64))

            v4 = cg.llvmEnumLabel("Hilti::AddrFamily::IPv4")
            v6 = cg.llvmEnumLabel("Hilti::AddrFamily::IPv6")

            result = cg.builder().select(builder.and_(cmp1, cmp2), v4, v6)
            cg.llvmStoreInTarget(self, result)

    */
}
