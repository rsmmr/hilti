// $Id$

#include "hilti.h"

hlt_stack* hlt_stack_new(int size, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! size )
        size = HLT_BOUND_FUNCTION_STACK_SIZE;

    hlt_stack* s = hlt_gc_malloc_non_atomic(sizeof(hlt_stack));

    if ( ! s ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    s->fptr = hlt_gc_malloc_non_atomic(size);
    if ( ! s->fptr ) {
        hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
        return 0;
    }

    s->eoss = s->fptr + size;

    return s;
}

void hlt_callable_register(hlt_callable* callable, struct hlt_exception** excpt, struct hlt_execution_context* ctx)
{
    if ( ctx->calls_num >= ctx->calls_alloced ) {
        ctx->calls_alloced = (ctx->calls_alloced * 2) + 2;
        ctx->calls = hlt_gc_realloc_non_atomic(ctx->calls, sizeof(hlt_callable *) * ctx->calls_alloced);

        if ( ! ctx->calls ) {
            hlt_set_exception(excpt, &hlt_exception_out_of_memory, 0);
            return;
        }
    }

    ctx->calls[ctx->calls_num++] = callable;
}

hlt_callable** __hlt_callable_next(struct hlt_execution_context* ctx, struct hlt_exception** excpt)
{
    // We optimize here for the case that the array is small so that
    // searching for the first non_empty field is not expensive. Also, the
    // typical use case for this functions is that we go through all queued
    // elements before adding further ones.

    if ( ctx->calls_first >= ctx->calls_num ) {
        return 0;
    }

    hlt_callable** p = &ctx->calls[ctx->calls_first++];

    if ( ctx->calls_first >= ctx->calls_num )
        // End reached, clear array.
        ctx->calls_first = ctx->calls_num = 0;

    return p;
}
