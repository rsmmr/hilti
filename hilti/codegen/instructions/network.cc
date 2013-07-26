
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

static llvm::Value* _isV4(CodeGen* cg, llvm::Value* op)
{
    auto c = cg->llvmExtractValue(op, 0);
    auto cmp1 = cg->builder()->CreateICmpEQ(c, cg->llvmConstInt(0, 64));

    auto v = cg->llvmExtractValue(op, 1);
    auto masked = cg->builder()->CreateAnd(v, cg->llvmConstInt(0xffffffff00000000, 64));
    auto cmp2 = cg->builder()->CreateICmpEQ(masked, cg->llvmConstInt(0, 64));

    auto w = cg->llvmExtractValue(op, 2);
    auto cmp3 = cg->builder()->CreateICmpUGE(w, cg->llvmConstInt(96, 8));

    auto and_ = cg->builder()->CreateAnd(cmp1, cmp2);
    return cg->builder()->CreateAnd(and_, cmp3);
}

static llvm::Value* _maskAddr(CodeGen* cg, llvm::Value* addr, llvm::Value* len)
{
    auto allbits = cg->llvmConstInt(0xFFFFFFFFFFFFFFFF, 64);
    auto zero = cg->llvmConstInt(0, 64);
    auto sixtyfour = cg->llvmConstInt(64, 64);
    auto onetwentyeight = cg->llvmConstInt(128, 64);
    len = cg->builder()->CreateSExt(len, cg->llvmTypeInt(64));

    auto a1 = cg->llvmExtractValue(addr, 0);
    auto shift = cg->builder()->CreateSub(sixtyfour, len);
    auto mask = cg->builder()->CreateShl(allbits, shift);
    auto cmp = cg->builder()->CreateICmpULT(len, sixtyfour);
    mask = cg->builder()->CreateSelect(cmp, mask, allbits);
    a1 = cg->builder()->CreateAnd(a1, mask);

    // Need to special case length zero because the shift isn't defined for
    // shifting 64 bits.
    cmp = cg->builder()->CreateICmpNE(len, zero);
    a1 = cg->builder()->CreateSelect(cmp, a1, zero);

    addr = cg->llvmInsertValue(addr, a1, 0);

    auto a2 = cg->llvmExtractValue(addr, 1);
    shift = cg->builder()->CreateSub(onetwentyeight, len);
    mask = cg->builder()->CreateShl(allbits, shift);
    cmp = cg->builder()->CreateICmpUGT(len, sixtyfour);
    mask = cg->builder()->CreateSelect(cmp, mask, zero);
    addr = cg->llvmInsertValue(addr, cg->builder()->CreateAnd(a2, mask), 1);

    return addr;
}

void StatementBuilder::visit(statement::instruction::network::Equal* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto v1 = cg()->llvmExtractValue(op1, 0);
    auto v2 = cg()->llvmExtractValue(op2, 0);
    auto cmp1 = cg()->builder()->CreateICmpEQ(v1, v2);

    v1 = cg()->llvmExtractValue(op1, 1);
    v2 = cg()->llvmExtractValue(op2, 1);
    auto cmp2 = cg()->builder()->CreateICmpEQ(v1, v2);

    v1 = cg()->llvmExtractValue(op1, 2);
    v2 = cg()->llvmExtractValue(op2, 2);
    auto cmp3 = cg()->builder()->CreateICmpEQ(v1, v2);

    auto result = cg()->builder()->CreateAnd(cmp1, cmp2);
    result = cg()->builder()->CreateAnd(result, cmp3);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::network::EqualAddr* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto len = cg()->llvmExtractValue(op1, 2);
    auto addr = _maskAddr(cg(), op2, len);

    auto v1 = cg()->llvmExtractValue(op1, 0);
    auto v2 = cg()->llvmExtractValue(addr, 0);
    auto cmp1 = cg()->builder()->CreateICmpEQ(v1, v2);

    v1 = cg()->llvmExtractValue(op1, 1);
    v2 = cg()->llvmExtractValue(addr, 1);
    auto cmp2 = cg()->builder()->CreateICmpEQ(v1, v2);

    auto result = cg()->builder()->CreateAnd(cmp1, cmp2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::network::Family* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto v4 = cg()->llvmEnum("Hilti::AddrFamily::IPv4");
    auto v6 = cg()->llvmEnum("Hilti::AddrFamily::IPv6");

    auto result = cg()->builder()->CreateSelect(_isV4(cg(), op1), v4, v6);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::network::Length* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    // For IPv4, we need to subtract 96.
    auto len = cg()->llvmExtractValue(op1, 2);
    auto lenv4 = cg()->builder()->CreateSub(len, cg()->llvmConstInt(96, 8));
    auto result = cg()->builder()->CreateSelect(_isV4(cg(), op1), lenv4, len);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::network::Prefix* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    CodeGen::constant_list elems;
    elems.push_back(cg()->llvmConstInt(255, 64));
    elems.push_back(cg()->llvmConstInt(255, 64));

    llvm::Value* result = cg()->llvmConstStruct(elems);
    result = cg()->llvmInsertValue(result, cg()->llvmExtractValue(op1, 0), 0);
    result = cg()->llvmInsertValue(result, cg()->llvmExtractValue(op1, 1), 1);

    cg()->llvmStore(i, result);
}

