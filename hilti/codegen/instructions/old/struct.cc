
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::struct::Get* i)
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
        (idx, ftype) = _getIndex(self)
        assert idx >= 0

        s = cg.llvmOp(self.op1())
        val = self.op1().type().refType().llvmGetField(cg, s, idx, location=self.location())
        cg.llvmStoreInTarget(self, val)

*/
}

void StatementBuilder::visit(statement::instruction::struct::Get* i)
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
        (idx, ftype) = _getIndex(self)
        assert idx >= 0

        s = cg.llvmOp(self.op1())
        val = self.op1().type().refType().llvmGetField(cg, s, idx, default=cg.llvmOp(self.op3()), location=self.location())
        cg.llvmStoreInTarget(self, val)

*/
}

void StatementBuilder::visit(statement::instruction::struct::IsSet* i)
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
        (idx, ftype) = _getIndex(self)
        assert idx >= 0

        s = cg.llvmOp(self.op1())

        # Check mask.
        zero = cg.llvmGEPIdx(0)
        addr = cg.builder().gep(s, [zero, zero])
        mask = cg.builder().load(addr)

        bit = cg.llvmConstInt(1<<idx, 32)
        isset = cg.builder().and_(bit, mask)

        notzero = cg.builder().icmp(llvm.core.IPRED_NE, isset, cg.llvmConstInt(0, 32))
        cg.llvmStoreInTarget(self, notzero)

*/
}

void StatementBuilder::visit(statement::instruction::struct::Set* i)
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
        (idx, ftype) = _getIndex(self)
        assert idx >= 0

        s = cg.llvmOp(self.op1())

        # Set mask bit.
        zero = cg.llvmGEPIdx(0)
        addr = cg.builder().gep(s, [zero, zero])
        mask = cg.builder().load(addr)
        bit = cg.llvmConstInt(1<<idx, 32)
        mask = cg.builder().or_(bit, mask)
        cg.llvmAssign(mask, addr)

        index = cg.llvmGEPIdx(idx + 1)
        addr = cg.builder().gep(s, [zero, index])
        cg.llvmAssign(cg.llvmOp(self.op3(), ftype), addr)

*/
}

void StatementBuilder::visit(statement::instruction::struct::Unset* i)
{

/*
    auto op1 = cg()->llvmValue(i->op1(), X);
    auto op2 = cg()->llvmValue(i->op2(), X);
*/

/*
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::X", args);
*/


/*
    def _codegen(self, cg):
        (idx, ftype) = _getIndex(self)
        assert idx >= 0

        s = cg.llvmOp(self.op1())

        # Clear mask bit.
        zero = cg.llvmGEPIdx(0)
        addr = cg.builder().gep(s, [zero, zero])
        mask = cg.builder().load(addr)
        bit = cg.llvmConstInt(~(1<<idx), 32)
        mask = cg.builder().and_(bit, mask)
        cg.llvmAssign(mask, addr)

*/
}

