
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::thread::GetContext* i)
{
    auto tctx = cg()->llvmCurrentThreadContext();
    auto ty = cg()->hiltiModule()->executionContext();
    assert(ty);

    tctx = cg()->builder()->CreateBitCast(tctx, cg()->llvmType(ty));
    cg()->llvmCctor(tctx, ty, false, "thread.get_context");
    cg()->llvmStore(i, tctx);
}

void StatementBuilder::visit(statement::instruction::thread::SetContext* i)
{
    auto ty = cg()->hiltiModule()->executionContext();
    assert(ty);

    auto op1 = cg()->llvmValue(i->op1(), builder::reference::type(ty));
    cg()->llvmSetCurrentThreadContext(ty, op1);
}


void StatementBuilder::visit(statement::instruction::thread::ThreadID* i)
{
    auto vid = cg()->llvmCurrentVID();
    cg()->llvmStore(i, vid);
}

// Promotes thread context from a source scope to destination scope. The
// validator has already ensured that this operation is to ok to do. Note
// that we must never modify an existing context. We can directly reuse it
// however if that does not require any modification.
llvm::Value* _promoteContext(CodeGen* cg, llvm::Value* tctx,
                             shared_ptr<type::Scope> src_scope, shared_ptr<type::Context> src_context,
                             shared_ptr<type::Scope> dst_scope, shared_ptr<type::Context> dst_context)
{

    // If both scope and context match, we don't need to do anything.

    if ( src_scope->equal(dst_scope) && src_context->Type::equal(dst_context) )
        return tctx;

    // Build the destination context field by field.

    auto new_tctx = cg->llvmStructNew(dst_context);

    for ( auto f : dst_context->fields() ) {
        if ( ! dst_scope->hasField(f->id()) )
            // Scope doesn't need it, leave unset.
            continue;

        auto val = cg->llvmStructGet(src_context, tctx, f->id()->name(), nullptr, nullptr);
        val = cg->llvmCoerceTo(val, src_context->lookup(f->id())->type(), f->type());
        cg->llvmStructSet(dst_context, new_tctx, f->id()->name(), val);
        cg->llvmDtor(val, f->type(), false, "thread.cc::_promoteContext");
    }

    cg->llvmDtorAfterInstruction(new_tctx, builder::reference::type(dst_context), false);

    return new_tctx;
}

void StatementBuilder::visit(statement::instruction::thread::Schedule* i)
{
    CodeGen::expr_list params;
    prepareCall(i->op1(), i->op2(), &params);

    auto func = cg()->llvmValue(i->op1());
    auto ftype = ast::as<type::Function>(i->op1()->type());
    auto job = cg()->llvmCallableBind(func, ftype, params);

    auto mgr = cg()->llvmThreadMgr();
    auto ctx = cg()->llvmExecutionContext();

    if ( i->op3() ) {
        // Destination thread given.
        auto vid = cg()->llvmValue(i->op3(), builder::integer::type(64));

        CodeGen::value_list vals = { mgr, vid, job };
        cg()->llvmCallC("__hlt_thread_mgr_schedule", vals, true, true);
    }

    else {
        // No destination given, use thread context. But first ensure that we
        // actually have one.

        auto tctx = cg()->llvmCurrentThreadContext();

        auto block_ok = cg()->newBuilder("ok");
        auto block_no_ctx = cg()->newBuilder("no-ctx");

        auto not_null = cg()->builder()->CreateICmpNE(tctx, cg()->llvmConstNull(tctx->getType()));

        cg()->llvmCreateCondBr(not_null, block_ok, block_no_ctx);

        cg()->pushBuilder(block_no_ctx);
        cg()->llvmRaiseException("Hilti::NoThreadContext", i->location());
        cg()->popBuilder();

        cg()->pushBuilder(block_ok);

        // Do the Promotion.

        auto decl = current<declaration::Function>();
        assert(decl);

        auto func = decl->function();
        assert(func);

        auto op1 = ast::as<expression::Function>(i->op1());
        assert(op1);

        auto callee = op1->function();
        assert(callee);
        assert(callee->module());

        auto src_scope = func->scope();
        auto src_context = func->module()->executionContext();
        auto dst_scope = callee->scope();
        auto dst_context = callee->module()->executionContext();

        tctx = _promoteContext(cg(), tctx, src_scope, src_context, dst_scope, dst_context);
        tctx = cg()->builder()->CreateBitCast(tctx, cg()->llvmTypePtr());

        auto ti = cg()->llvmRtti(dst_context);
        CodeGen::value_list vals = { mgr, ti, tctx, job };
        cg()->llvmCallC("__hlt_thread_mgr_schedule_tcontext", vals, true, true);
    }

    cg()->llvmDtor(job, builder::reference::type(builder::callable::typeAny()), false, "thread.schedule");
}
