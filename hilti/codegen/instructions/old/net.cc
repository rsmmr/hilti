
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::net::Family* i)
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
        v4 = cg.llvmEnumLabel("Hilti::AddrFamily::IPv4")
        v6 = cg.llvmEnumLabel("Hilti::AddrFamily::IPv6")
        result = cg.builder().select(_isv4(cg, op1), v4, v6)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::net::Length* i)
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
        # For IPv4, we need to subtract 96.
        op1 = cg.llvmOp(self.op1())
        len = cg.llvmExtractValue(op1, 2)
        lenv4 = cg.builder().sub(len, cg.llvmConstInt(96, 8))
        result = cg.builder().select(_isv4(cg, op1), lenv4, len)
        cg.llvmStoreInTarget(self, result)

*/
}

void StatementBuilder::visit(statement::instruction::net::Prefix* i)
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
        a = llvm.core.Constant.struct([cg.llvmConstInt(255, 64)] * 2)
        a = cg.llvmInsertValue(a, 0, cg.llvmExtractValue(op1, 0))
        a = cg.llvmInsertValue(a, 1, cg.llvmExtractValue(op1, 1))
        cg.llvmStoreInTarget(self, a)

*/
}

