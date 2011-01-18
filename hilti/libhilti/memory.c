/* $Id
 *
 * GC-based replaced functions for memory management.
 *
 * Todo: Well, currently these are actually *not* garbage collected ...
 *
 */

#include "hilti.h"

// TODO: Using just these doesn't produce reliabley results currently, so we
// also configure the collector at compile time to simply replace all memory
// functions with its own versions. See libhilti/scripts/do-build. However,
// we should figure out at some point why that's necessary. Part of the
// problem is probably the 3rdparty code, which doesn't use the hlt_gc_*
// functions ...

#if USE_GC

void __hlt_init_gc()
{
    GC_INIT();

    // Disabled for now.
    //
    // GC_enable_incremental();
}

void* hlt_gc_malloc_atomic(uint64_t n)
{
    return GC_MALLOC_ATOMIC(n);
}

void* hlt_gc_malloc_non_atomic(uint64_t n)
{
    return GC_MALLOC(n);
}

void* hlt_gc_calloc_atomic(uint64_t count, uint64_t n)
{
    return GC_MALLOC_ATOMIC(count * n);
}

void* hlt_gc_calloc_non_atomic(uint64_t count, uint64_t n)
{
    return GC_MALLOC(count * n);
}

void* hlt_gc_realloc_atomic(void* ptr, uint64_t n)
{
    return GC_REALLOC(ptr, n);
}

void* hlt_gc_realloc_non_atomic(void* ptr, uint64_t n)
{
    return GC_REALLOC(ptr, n);
}

static void _finalizer(void* obj, void* client_data)
{
    (*((hlt_gc_finalizer_func*)client_data))(obj);
}

void hlt_gc_register_finalizer(void* obj, hlt_gc_finalizer_func* func)
{
    GC_REGISTER_FINALIZER(obj, _finalizer, func, 0, 0);
}

#else // For testing, without GC support. 

void __hlt_init_gc()
{
}

void* hlt_gc_malloc_atomic(uint64_t n)
{
    return malloc(n);
}

void* hlt_gc_malloc_non_atomic(uint64_t n)
{
    return malloc(n);
}

void* hlt_gc_calloc_atomic(uint64_t count, uint64_t n)
{
    return calloc(count, n);
}

void* hlt_gc_calloc_non_atomic(uint64_t count, uint64_t n)
{
    return calloc(count, n);
}

void* hlt_gc_realloc_atomic(void* ptr, uint64_t n)
{
    return realloc(ptr, n);
}

void* hlt_gc_realloc_non_atomic(void* ptr, uint64_t n)
{
    return realloc(ptr, n);
}

void hlt_gc_register_finalizer(void* obj, hlt_gc_finalizer_func* func)
{
    // Can't do finalizers.
}

#endif
