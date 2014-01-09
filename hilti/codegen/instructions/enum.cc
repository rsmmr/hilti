
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"
#include "libhilti/enum.h"

using namespace hilti;
using namespace codegen;

#if 0
static llvm::Value* _makeVal(CodeGen* cg, bool undef, bool have_value, uint64_t val)
{
    auto flags = cg->llvmConstInt((undef ? HLT_ENUM_UNDEF : 0) + (have_value ? HLT_ENUM_HAS_VAL : 0), 64);
    auto llvm_val = cg->llvmConstInt(val, 64);

    CodeGen::constant_list elems;
    elems.push_back(flags);
    elems.push_back(llvm_val);
    return cg->llvmConstStruct(elems, true);
}
#endif

static llvm::Value* _makeValLLVM(CodeGen* cg, llvm::Value* undef, llvm::Value* has_val, llvm::Value* val)
{
    auto undef_set = cg->builder()->CreateICmpNE(undef, cg->llvmConstInt(0, 1));
    auto has_val_set = cg->builder()->CreateICmpNE(has_val, cg->llvmConstInt(0, 1));

    undef = cg->builder()->CreateSelect(undef_set, cg->llvmConstInt(HLT_ENUM_UNDEF, 64), cg->llvmConstInt(0, 64));
    has_val = cg->builder()->CreateSelect(has_val_set, cg->llvmConstInt(HLT_ENUM_HAS_VAL, 64), cg->llvmConstInt(0, 64));

    auto flags = cg->builder()->CreateOr(undef, has_val);

    CodeGen::value_list elems;
    elems.push_back(flags);
    elems.push_back(val);
    return cg->llvmValueStruct(elems, true);
}

static llvm::Value* _getUndef(CodeGen* cg, llvm::Value* op)
{
    auto flags = cg->llvmExtractValue(op, 0);
    auto bit = cg->builder()->CreateAnd(flags, cg->llvmConstInt(HLT_ENUM_UNDEF, 64));
    return cg->builder()->CreateICmpNE(bit, cg->llvmConstInt(0, 64));
}

#if 0
static llvm::Value* _getHaveVal(CodeGen* cg, llvm::Value* op)
{
    auto flags = cg->llvmExtractValue(op, 0);
    auto bit = cg->builder()->CreateAnd(flags, cg->llvmConstInt(HLT_ENUM_HAS_VAL, 64));
    return cg->builder()->CreateICmpNE(bit, cg->llvmConstInt(0, 64));
}
#endif

static llvm::Value* _getVal(CodeGen* cg, llvm::Value* op)
{
    return cg->llvmExtractValue(op, 1);
}

void StatementBuilder::visit(statement::instruction::enum_::Equal* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto undef1 = _getUndef(cg(), op1);
    auto undef2 = _getUndef(cg(), op2);

    auto val1 = _getVal(cg(), op1);
    auto val2 = _getVal(cg(), op2);

    auto undef_or = cg()->builder()->CreateOr(undef1, undef2);
    auto undef_and = cg()->builder()->CreateAnd(undef1, undef2);
    auto vals = cg()->builder()->CreateICmpEQ(val1, val2);

    auto result = cg()->builder()->CreateSelect(undef_or, undef_and, vals);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::enum_::FromInt* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto block_cont = cg()->newBuilder("enum-cont");

    auto block_known = cg()->pushBuilder("enum-known");
    cg()->llvmCreateBr(block_cont);
    cg()->popBuilder();

    auto block_unknown = cg()->pushBuilder("enum-unknown");
    cg()->llvmCreateBr(block_cont);
    cg()->popBuilder();

    op1 = cg()->builder()->CreateZExt(op1, cg()->llvmTypeInt(64));
    auto switch_ = builder()->CreateSwitch(op1, block_unknown->GetInsertBlock());

    std::set<int> have_vals;

    for ( auto l : ast::as<type::Enum>(i->target()->type())->labels() ) {
        auto label = l.first;
        auto value = l.second;

        // If one value has multiple labels, we just take the first.
        if ( have_vals.find(value) != have_vals.end() )
            continue;

        switch_->addCase(cg()->llvmConstInt(value, 64), block_known->GetInsertBlock());
        have_vals.insert(value);
    }

    cg()->pushBuilder(block_cont);

    auto phi = builder()->CreatePHI(cg()->llvmTypeInt(1), 2);
    phi->addIncoming(cg()->llvmConstInt(0, 1), block_known->GetInsertBlock());
    phi->addIncoming(cg()->llvmConstInt(1, 1), block_unknown->GetInsertBlock());

    auto result = _makeValLLVM(cg(), phi, cg()->llvmConstInt(1, 1), op1);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::enum_::ToInt* i)
{
    auto ty_target = as<type::Integer>(i->target()->type());

    auto op1 = cg()->llvmValue(i->op1());
    auto undef = _getUndef(cg(), op1);
    auto val = _getVal(cg(), op1);
    val = builder()->CreateTrunc(val, cg()->llvmTypeInt(ty_target->width()));
    auto result = cg()->builder()->CreateSelect(undef, cg()->llvmConstInt(-1, ty_target->width()), val);

    cg()->llvmStore(i, result);
}
