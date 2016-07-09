
#include <stdio.h>

#include "config.h"
#include "context.h"
#include "exceptions.h"
#include "globals.h"
#include "linker.h"
#include "memory_.h"
#include "profiler.h"
#include "rtti.h"
#include "timer.h"

hlt_execution_context* __hlt_execution_context_new_ref(hlt_vthread_id vid, int8_t run_module_init)
{
    hlt_execution_context* ctx = (hlt_execution_context*)hlt_malloc(sizeof(hlt_execution_context) +
                                                                    __hlt_globals()->globals_size);

    ctx->vid = vid;
    ctx->nullbuffer = __hlt_memory_nullbuffer_new(); // init first
    ctx->excpt = 0;
    ctx->fiber = 0;
    ctx->fiber_pool = __hlt_fiber_pool_new();
    ctx->worker = 0;
    ctx->tcontext = 0;
    ctx->tcontext_type = 0;
    ctx->pstate = 0;
    ctx->blockable = 0;
    ctx->tmgr = hlt_timer_mgr_new(&ctx->excpt, ctx);
    GC_CCTOR(ctx->tmgr, hlt_timer_mgr, ctx);

    __hlt_globals_init(ctx);

    if ( run_module_init )
        __hlt_modules_init(ctx);

    if ( ctx->excpt )
        hlt_exception_print_uncaught_abort(ctx->excpt, ctx);

    return ctx;
}

void hlt_execution_context_delete(hlt_execution_context* ctx)
{
    hlt_exception* excpt = 0;

    // Do this first, it may still need the context.
    hlt_timer_mgr_expire(ctx->tmgr, 0, &excpt, ctx);
    GC_DTOR(ctx->tmgr, hlt_timer_mgr, ctx);

    __hlt_globals_dtor(ctx);

    GC_DTOR(ctx->excpt, hlt_exception, ctx);

    if ( ctx->fiber )
        hlt_fiber_delete(ctx->fiber, ctx);

    if ( ctx->pstate )
        __hlt_profiler_state_delete(ctx->pstate);

    if ( ctx->tcontext ) {
        GC_DTOR_GENERIC(&ctx->tcontext, ctx->tcontext_type, ctx);
    }

    __hlt_fiber_pool_delete(ctx->fiber_pool);

    if ( ctx->nullbuffer )
        __hlt_memory_nullbuffer_delete(ctx->nullbuffer, ctx);

    hlt_free(ctx);
}

void __hlt_context_set_exception(hlt_execution_context* ctx, hlt_exception* excpt)
{
    GC_ASSIGN(ctx->excpt, excpt, hlt_exception, ctx);
}

hlt_exception* __hlt_context_get_exception(hlt_execution_context* ctx)
{
    return ctx->excpt;
}

void __hlt_context_clear_exception(hlt_execution_context* ctx)
{
    GC_CLEAR(ctx->excpt, hlt_exception, ctx);
}

hlt_fiber* __hlt_context_get_fiber(hlt_execution_context* ctx)
{
    return ctx->fiber;
}

void __hlt_context_set_fiber(hlt_execution_context* ctx, hlt_fiber* fiber)
{
    ctx->fiber = fiber;
}

void* __hlt_context_get_thread_context(hlt_execution_context* ctx)
{
    return ctx->tcontext;
}

void __hlt_context_set_thread_context(hlt_execution_context* ctx, hlt_type_info* type, void* tctx)
{
    GC_DTOR_GENERIC(&ctx->tcontext, ctx->tcontext_type, ctx);
    ctx->tcontext = tctx;
    ctx->tcontext_type = type;
    GC_CCTOR_GENERIC(&ctx->tcontext, ctx->tcontext_type, ctx);
}

hlt_vthread_id __hlt_context_get_vid(hlt_execution_context* ctx)
{
    return ctx->vid;
}

void __hlt_context_set_blockable(hlt_execution_context* ctx, __hlt_thread_mgr_blockable* b)
{
    ctx->blockable = b;
}
