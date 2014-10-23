
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::overlay::Attach* i)
{
    auto itype = builder::iterator::type(builder::bytes::type());
    auto otype = ast::as<type::Overlay>(i->op1()->type());
    auto ov = cg()->llvmValue(i->op1());
    auto iter = cg()->llvmValue(i->op2());

    // We don't reference count here because overlays are stored on the
    // stack.

    // Save the starting offset.
    ov = cg()->llvmInsertValue(ov, iter, 0);

    // Clear the cache.
    auto end = cg()->llvmIterBytesEnd();

    for ( int j = 1; j < 1 + otype->numDependencies(); ++j )
        ov = cg()->llvmInsertValue(ov, end, j);

    // Need to rewrite back into the original value.
    cg()->llvmStore(i->op1(), ov);
}

// Generates the unpacking code for one field, assuming all dependencies are
// already resolved. Returns tuple (new overlay, unpacked val) where val is
// not ref'ed.
static std::pair<llvm::Value*, llvm::Value*> _emitOne(CodeGen* cg, shared_ptr<type::Overlay> otype, llvm::Value* ov, shared_ptr<type::overlay::Field> field, llvm::Value* offset0, llvm::Value* b, const Location& l)
{
    auto itype = builder::iterator::type(builder::bytes::type());
    auto btype = builder::reference::type(builder::bytes::type());

    llvm::Value* begin = nullptr;

    if ( field->startOffset() == 0 ) {
        // Starts right at beginning.
        if ( b ) {
            auto arg1 = builder::codegen::create(btype, b);
            CodeGen::expr_list args = { arg1 };
            begin = cg->llvmCall("hlt::bytes_begin", args);
        }

        else {
            assert(offset0);
            begin = offset0;
        }
    }

    else if ( field->startOffset() > 0 ) {
        // Static offset. We can calculate the starting position directly.
        if ( b ) {
            auto arg1 = builder::codegen::create(btype, b);
            auto arg2 = builder::integer::create(field->startOffset());
            CodeGen::expr_list args = { arg1, arg2 };
            begin = cg->llvmCall("hlt::bytes_offset", args);
        }

        else {
            auto arg1 = builder::codegen::create(itype, offset0);
            auto arg2 = builder::integer::create(field->startOffset());
            CodeGen::expr_list args = { arg1, arg2 };
            begin = cg->llvmCall("hlt::iterator_bytes_incr_by", args);
        }
    }

    else {
        assert(! b);
        assert(ov);

        // Offset is not static but our immediate dependency must have
        // already been calculated at this point so we take it's end position
        // out of the cache.
        auto dep = field->dependents().back()->depIndex();
        assert(dep > 0);
        begin = cg->llvmExtractValue(ov, dep);
    }

    llvm::Value* fmt = cg->llvmValue(field->format());
    llvm::Value* arg = nullptr;
    shared_ptr<Type> arg_type = nullptr;

    if ( field->formatArg() ) {
        arg = cg->llvmValue(field->formatArg());
        arg_type = field->formatArg()->type();
    }

    auto unpacked = cg->llvmUnpack(field->type(), begin, cg->llvmIterBytesEnd(), fmt, arg, arg_type, l);
    auto val = unpacked.first;
    auto end = unpacked.second;

    if ( field->depIndex() >= 0 )
        ov = cg->llvmInsertValue(ov, end, field->depIndex());

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

    auto one = _emitOne(cg, otype, phi, dep, offset0, 0, l);

    // Leave builder on stack.

    return one.first;
}

void StatementBuilder::visit(statement::instruction::overlay::Get* i)
{
    auto otype = ast::as<type::Overlay>(i->op1()->type());
    auto itype = builder::iterator::type(builder::bytes::type());

    auto ov = cg()->llvmValue(i->op1());

    auto cexpr = ast::as<expression::Constant>(i->op2());
    assert(cexpr);
    auto cval = ast::as<constant::String>(cexpr->constant());
    assert(cval);

    auto field = otype->field(cval->value());

    if ( i->op3() ) {
        // Working in detached mode.
        auto op3 = cg()->llvmValue(i->op3());
        auto one = _emitOne(cg(), otype, ov, field, 0, op3, i->location());
        cg()->llvmStore(i, one.second);

        // Ignore overlay, it won't have changed.
    }

    else {
        // Standard mode, make sure we're attached.
        auto block_attached = cg()->newBuilder("attached");
        auto block_notattached = cg()->newBuilder("not-attached");

        auto offset0 = cg()->llvmExtractValue(ov, 0);

        CodeGen::expr_list args = { builder::codegen::create(itype, offset0), builder::codegen::create(itype, cg()->llvmIterBytesEnd()) };
        auto is_end = cg()->llvmCall("hlt::iterator_bytes_eq", args);
        cg()->llvmCreateCondBr(is_end, block_notattached, block_attached);

        cg()->pushBuilder(block_notattached);
        cg()->llvmRaiseException("Hilti::OverlayNotAttached", i->location());
        cg()->popBuilder();

        cg()->pushBuilder(block_attached);

        ov = _makeDep(cg(), otype, ov, field, offset0, i->location());
        auto one = _emitOne(cg(), otype, ov, field, offset0, 0, i->location());
        cg()->llvmStore(i, one.second);

        // Don't dtor the old value.
        cg()->llvmStore(i->op1(), one.first, false);

        // Leave builder on stack.
    }
}

void StatementBuilder::visit(statement::instruction::overlay::GetStatic* i)
{
    auto otype = ast::checkedCast<type::Overlay>(typedType(i->op1()));
    auto cexpr = ast::checkedCast<expression::Constant>(i->op2());
    auto cval = ast::checkedCast<constant::String>(cexpr->constant());
    auto bytes = cg()->llvmValue(i->op3());

    auto field = otype->field(cval->value());

    auto one = _emitOne(cg(), otype, 0, field, 0, bytes, i->location());

    cg()->llvmStore(i, one.second);
}
