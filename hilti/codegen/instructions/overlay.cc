
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::overlay::Attach* i)
{
    auto itype = builder::iterator::type(builder::bytes::type());
    auto otype = ast::as<type::Overlay>(i->op1()->type());
    auto ov = cg()->llvmValue(i->op1());
    auto iter = cg()->llvmValue(i->op2());

    auto old_iter= cg()->llvmExtractValue(ov, 0);
    cg()->llvmDtor(old_iter, itype, false, "overlay-attach");

    // Save the starting offset.
    ov = cg()->llvmInsertValue(ov, iter, 0);
    cg()->llvmCctor(iter, itype, false, "overlay-attach");

    // Clear the cache.
    auto end = cg()->llvmIterBytesEnd();

    for ( int j = 1; j < 1 + otype->numDependencies(); ++j ) {
        old_iter = cg()->llvmExtractValue(ov, j);
        cg()->llvmDtor(old_iter, itype, false, "overlay-attach");

        ov = cg()->llvmInsertValue(ov, end, j); // Probably unnecessary, but can't hurt.
        cg()->llvmCctor(end, itype, false, "overlay-attach");
    }


    // Need to rewrite back into the original value.
    cg()->llvmStore(i->op1(), ov);
}

// Generates the unpacking code for one field, assuming all dependencies are
// already resolved. Returns tuple (new overlay, unpacked val) where val is
// already ref'ed.
static std::pair<llvm::Value*, llvm::Value*> _emitOne(CodeGen* cg, shared_ptr<type::Overlay> otype, llvm::Value* ov, shared_ptr<type::overlay::Field> field, llvm::Value* offset0, const Location& l)
{
    auto itype = builder::iterator::type(builder::bytes::type());

    llvm::Value* begin = nullptr;
    auto unref_begin = false;

    if ( field->startOffset() == 0 ) {
        // Starts right at beginning.
        begin = offset0;
    }

    else if ( field->startOffset() > 0 ) {
        // Static offset. We can calculate the starting position directly.
        auto arg1 = builder::codegen::create(itype, offset0);
        auto arg2 = builder::integer::create(field->startOffset());
        CodeGen::expr_list args = { arg1, arg2 };
        begin = cg->llvmCall("hlt::iterator_bytes_incr_by", args);
        unref_begin = true;
    }

    else {
        // Offset is not static but our immediate dependency must have
        // already been calculated at this point so we take it's end position
        // out of the cache.
        auto dep = field->dependents().back()->depIndex();
        assert(dep > 0);
        begin = cg->llvmExtractValue(ov, dep);
    }

    llvm::Value* fmt = cg->llvmValue(field->format());
    llvm::Value* arg = nullptr;

    if ( field->formatArg() )
        arg = cg->llvmValue(field->formatArg());

    auto unpacked = cg->llvmUnpack(field->type(), begin, cg->llvmIterBytesEnd(), fmt, arg, field->format()->type(), l);
    auto val = unpacked.first;
    auto end = unpacked.second;

    if ( field->depIndex() >= 0 ) {
        auto old_iter = cg->llvmExtractValue(ov, field->depIndex());
        cg->llvmDtor(old_iter, itype, false, "overlay-emit-one-0");
        ov = cg->llvmInsertValue(ov, end, field->depIndex());
    }

    else
        cg->llvmDtor(end, itype, false, "overlay-emit-one-1");

    if ( unref_begin )
        cg->llvmDtor(begin, itype, false, "overlay-emit-one-2");

    return std::make_pair(ov, val);
}

// Generate code for all depedencies. Returns new overlay.
static llvm::Value* _makeDep(CodeGen* cg, shared_ptr<type::Overlay> otype, llvm::Value* ov, shared_ptr<type::overlay::Field> field, llvm::Value* offset0, const Location& l)
{
    if ( ! field->dependents().size() )
        return ov;

    auto itype = builder::iterator::type(builder::bytes::type());
    auto dep = field->dependents().back();

    auto block_cont = cg->newBuilder(::util::fmt("have-%s", dep->name()->name().c_str()));
    auto block_cached = cg->newBuilder(::util::fmt("cached-%s", dep->name()->name().c_str()));
    auto block_notcached = cg->newBuilder(::util::fmt("notcached-%s", dep->name()->name().c_str()));

    auto iter = cg->llvmExtractValue(ov, dep->depIndex());
    CodeGen::expr_list args = { builder::codegen::create(itype, iter), builder::codegen::create(itype, cg->llvmIterBytesEnd()) };
    auto is_end = cg->llvmCall("hlt::iterator_bytes_eq", args);
    cg->llvmCreateCondBr(is_end, block_notcached, block_cached);

    cg->pushBuilder(block_notcached);
    auto ov1 = _makeDep(cg, otype, ov, dep, offset0, l);
    auto notcached_exit_block = cg->builder();
    cg->llvmCreateBr(block_cont);
    cg->popBuilder();

    cg->pushBuilder(block_cached);
    auto ov2 = ov;
    cg->llvmCreateBr(block_cont);
    cg->popBuilder();

    cg->pushBuilder(block_cont);
    auto phi = cg->builder()->CreatePHI(ov->getType(), 2);
    phi->addIncoming(ov1, notcached_exit_block->GetInsertBlock());
    phi->addIncoming(ov2, block_cached->GetInsertBlock());

    auto one = _emitOne(cg, otype, phi, dep, offset0, l);

    // Leave builder on stack.

    return one.first;
}

void StatementBuilder::visit(statement::instruction::overlay::Get* i)
{
    auto otype = ast::as<type::Overlay>(i->op1()->type());
    auto itype = builder::iterator::type(builder::bytes::type());

    auto ov = cg()->llvmValue(i->op1());
    auto offset0 = cg()->llvmExtractValue(ov, 0);

    // Make sure we're attached.
    auto block_attached = cg()->newBuilder("attached");
    auto block_notattached = cg()->newBuilder("not-attached");

    CodeGen::expr_list args = { builder::codegen::create(itype, offset0), builder::codegen::create(itype, cg()->llvmIterBytesEnd()) };
    auto is_end = cg()->llvmCall("hlt::iterator_bytes_eq", args);
    cg()->llvmCreateCondBr(is_end, block_notattached, block_attached);

    cg()->pushBuilder(block_notattached);
    cg()->llvmRaiseException("Hilti::OverlayNotAttached", i->location());
    cg()->popBuilder();

    cg()->pushBuilder(block_attached);

    auto cexpr = ast::as<expression::Constant>(i->op2());
    assert(cexpr);
    auto cval = ast::as<constant::String>(cexpr->constant());
    assert(cval);

    auto field = otype->field(cval->value());

    ov = _makeDep(cg(), otype, ov, field, offset0, i->location());
    auto one = _emitOne(cg(), otype, ov, field, offset0, i->location());
    cg()->llvmStore(i, one.second);

    // Don't dtor the old value.
    cg()->llvmStore(i->op1(), one.first, false);

    // Leave builder on stack.
}

