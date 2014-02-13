
#include <memory.h>

#include "clone.h"
#include "hutil.h"
#include "rtti.h"
#include "string_.h"
#include "exceptions.h"

// We break out the fastpaths to help the compiler inline.
static int8_t _fastpath_clone(void* dst, const hlt_type_info* ti, void* srcp);
static void _slowpath_clone(void* dst, const hlt_type_info* ti, void* srcp, int16_t type, hlt_vthread_id vid, hlt_exception** excpt, hlt_execution_context* ctx);
static void _slowpath_clone_recursive(void* dst, const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx);

void hlt_clone(void* dstp, const hlt_type_info* ti, void* srcp, int16_t type, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( _fastpath_clone(dstp, ti, srcp) )
        return;

    _slowpath_clone(dstp, ti, srcp, type, ctx->vid, excpt, ctx);
}

void hlt_clone_for_thread(void* dstp, const hlt_type_info* ti, void* srcp, hlt_vthread_id vid, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( _fastpath_clone(dstp, ti, srcp) )
        return;

    _slowpath_clone(dstp, ti, srcp, HLT_CLONE_DEEP, vid, excpt, ctx);
}

void __hlt_clone(void* dstp, const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( _fastpath_clone(dstp, ti, srcp) )
        return;

    _slowpath_clone_recursive(dstp, ti, srcp, cstate, excpt, ctx);
}

void __hlt_clone_init_in_thread(__hlt_init_in_thread_callback cb, const hlt_type_info* ti, void* dstp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ctx->vid == cstate->vid ) {
        // Right thread, can call it directly.
        (*cb)(ti, dstp, excpt, ctx);
        return;
    }

    // TODO: Calling in a different target thread not yet implemented.
    //
    // We need to record these somewhere (probably inside either the context
    // or the corresponding native thread structure), and then execute once
    // we're back in that thread. Details to be figured out.
    hlt_string msg = hlt_string_from_asciiz("_hlt_clone_init_in_thread for different target threat", excpt, ctx);
    hlt_set_exception(excpt, &hlt_exception_not_implemented, msg);
}

int8_t _fastpath_clone(void* dstp, const hlt_type_info* ti, void* srcp)
{
    if ( ti->atomic ) {
        // Short-cut the easy case.
        memcpy(dstp, srcp, ti->size);
        return 1;
    }

    if ( ti->gc && ! *(void**)srcp ) {
        *(void **)dstp = 0;
        return 1;
    }

    return 0;
}

static void _slowpath_clone(void* dstp, const hlt_type_info* ti, void* srcp, int16_t type, hlt_vthread_id vid, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_clone_state cstate;
    __hlt_pointer_map_init(&cstate.objs);
    cstate.type = type;
    cstate.vid = vid;
    cstate.level = -1;

    _slowpath_clone_recursive(dstp, ti, srcp, &cstate, excpt, ctx);

    __hlt_pointer_map_destroy(&cstate.objs);
}

static void _slowpath_clone_recursive(void* dstp, const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    ++cstate->level;

    if ( ti->gc ) {
        const void* cached = __hlt_pointer_map_lookup(&cstate->objs, *(void**)srcp);

        if ( cached ) {
            *(const void **)dstp = cached;
            GC_CCTOR_GENERIC(dstp, ti);
            return;
        }
    }

    if ( cstate->type == HLT_CLONE_SHALLOW && cstate->level > 0 ) {
        if ( ti->gc )
            memcpy(dstp, srcp, sizeof(void *));
        else
            memcpy(dstp, srcp, ti->size);

        GC_CCTOR_GENERIC(dstp, ti);
        return;
    }

    if ( cstate->type == HLT_CLONE_DEEP || cstate->level == 0) {

        if ( ti->gc ) {
            if ( ! ti->clone_alloc ) {
                hlt_string tag = hlt_string_from_asciiz(ti->tag, excpt, ctx);
                hlt_set_exception(excpt, &hlt_exception_cloning_not_supported, tag);
                return;
            }

            *(void **)dstp = (*ti->clone_alloc)(ti, srcp, cstate, excpt, ctx);

            if ( *excpt )
                return;
        }

        __hlt_pointer_map_insert(&cstate->objs, *(void**)srcp, *(void**)dstp);

        if ( ! ti->clone_init ) {
            hlt_string tag = hlt_string_from_asciiz(ti->tag, excpt, ctx);
            hlt_set_exception(excpt, &hlt_exception_cloning_not_supported, tag);
            return;
        }

        (*ti->clone_init)(dstp, ti, srcp, cstate, excpt, ctx);

        return;
    }

    hlt_set_exception(excpt, &hlt_exception_internal_error, hlt_string_from_asciiz("unknown clone type", excpt, ctx));
}

