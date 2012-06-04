
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::overlay::Attach* i)
{
    auto otype = i->op1()->type();
    auto ov = cg()->llvmVal(i->op1());
    auto iter = cg()->llvmVal(i->op2());

    // Save the starting offset.
    ov = cg()->llvmInsertValue(ov, iter, 0);

    // Clear the cache.
    auto end = cg()->llvmIterBytesEnd();

    for ( int j = 1; j <= 1 + otype()->numDependencies(); ++j )
        ov = cg()->llvmInsertValue(ov, end, j);

    // Need to rewrite back into the original value.
    cg()->llvmStore(i->op1(), ov);
}

// Generates the unpacking code for one field, assuming all dependencies are
// already resolved. Returns tuple (new overlay, unpacked val).
static std::pair<llvm::Value*, llvm::Value*> _emitOne(CodeGen* cg, shared_ptr<Overlay> otype, llvm::Value* ov, shared_ptr<type::overlay::Field> field, llvm::Value* offset0, const Location& l)
{
    auto itype = cg()->llvmType(builder::iterator::type(builder::bytes::type()));

    llvm::Value* begin = nullptr;
    auto unref_begin = false;

    if ( field->startOffset() == 0 ) {
        // Starts right at beginning.
        begin = offset0;
    }

    else if ( field->startOffset() > 0 ) {
        // Static offset. We can calculate the starting position directly.
        arg1 = builder::codegen::create(itype, offset0);
        arg2 = builder::codegen::create(builder::integer::type(32), field->startOffset());
        begin = cg.llvmCallC("hlt::bytes_pos_incr_by", [arg1, arg2]);
        unref_begin = true;
    }

    else {
        // Offset is not static but our immediate dependency must have
        // already been calculated at this point so we take it's end position
        // out of the cache.
        auto dep = otype->dependents().back()->depIndex();
        assert(dep > 0);
        begin = cg->llvmExtractValue(ov, dep);
    }

    llvm::Value* fmt = cg->llvmValue(field->format());
    llvm::Value* arg = nullptr;

    if ( field->formatArg() )
        arg = cg->llvmValue(field->formatArg());

    auto unpacked = cg->llvmUnpack(field->type(), begin, None, fmt, arg, l);
    auto val = unpacked.first;
    auto end = unpacked.second;

    if ( field->depIndex() >= 0 )
        ov = cg->llvmInsertValue(ov, end, field.depIndex());

    return std::make_pair(ov, val);
}

// Generate code for all depedencies. Returns new overlay.
static llvm::Value* _makeDep(CodeGen* cg, shared_ptr<Overlay> otype, llvm::Value* ov, shared_ptr<type::overlay::Field> field, llvm::Value* offset0, const Location& l)
{
    if ( ! field->dependents() )
        return ov;

    auto itype = cg()->llvmType(builder::iterator::type(builder::bytes::type()));
    auto dep = otype->field(field->dependents().back());

    auto block_cont = cg->newBuilder(util::fmt("have-%s", dep->name()->name().c_str()));
    auto block_cached = cg->newBuilder(util::fmt("cached-%s", dep->name()->name().c_str()));
    auto block_notcached = cg->newBuilder(util::fmt("notcached-%s", dep->name()->name().c_str()));

    auto iter = cg->llvmExtractValue(ov, dep->depIndex());
    CodeGen::expr_list arg = { builder::codegen::create(itype, iter), builder::codegen::create(itype, cg->llvmIterBytesEnd()) };
    auto is_end = cg->llvmCall("hlt::bytes_pos_eq", args);
    cg->llvmCreateCondBr(is_end, block_notcached, block_cont);

    cg->pushBuilder(block_notcached);
    ov = _makeDep(otype, ov, dep, offset0, l);
    cg->llvmCreateBr(block_cont);
    cg->popBuilder();

    cg->pushBuilder(block_cont);
    auto one = _emitOne(cg, otype, ov, dep, offset0, l);

    // Leave builder on stack.

    return ov.first;
}

void StatementBuilder::visit(statement::instruction::overlay::Get* i)
{
    auto otype = i->op1()->type();
    auto ov = cg()->llvmVal(i->op1());
    auto offset0 = cg()->llvmExtractValue(ov, 0);

    // Make sure we're attached.
    auto block_attached = cg()->newBuilder("attached");
    auto block_notattached = cg()->newBuilder("not-attached");

    CodeGen::expr_list arg = { builder::codegen::create(itype, offset0), builder::codegen::create(itype, cg->llvmIterBytesEnd()) };
    auto is_end = cg->llvmCall("hlt::bytes_pos_eq", args);
    cg->llvmCreateCondBr(is_end, block_notattached, block_attached);

    cg()->pushBuilder(block_notattached);
    cg()->llvmRaiseException("Hilti::OverlayNotAttached", i->location());
    cg()->popBuilder()

    cg.pushBuilder(block_attached);

    auto cexpr = ast::as<expression::Constant>(i->op2());
    assert(cexpr);
    auto cval = ast::as<constant::String>(cexpr->constant());
    assert(cval);

    auto field = otype->field(cval->value());

    ov = _makeDep(cg(), otype, ov, field, offset0, i->location());
    auto one = _emitOne(ov, field);
    cg->llvmStore(i, one.second);

    // Leave builder on stack.
}

