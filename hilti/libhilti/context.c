// $Id$

#include "context.h"
#include "thread_context.h"

void __hlt_execution_context_init(hlt_execution_context* ctx)
{
    ctx->globals = 0; // No support for globals yet. 
}

void __hlt_execution_context_done(hlt_execution_context* ctx)
{
    // Nothing to do currently.
}

hlt_execution_context* hlt_current_execution_context()
{
    if ( ! hlt_is_multi_threaded() )
        return &__hlt_global_execution_context;
    else {
        hlt_thread_context* tctx = __hlt_get_current_thread_context();
        uint32_t tid = __hlt_get_current_thread_id();
        if ( ! tid )
            return &tctx->main_thread->exec_context;
        else
            return &tctx->worker_threads[tid].exec_context;
    }
}
