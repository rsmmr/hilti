/* $Id
 *
 * GC-based replaced functions for memory management.
 *
 * Todo: Well, currently these are actually *not* garbage collected ...
 *
 */

#include "hilti.h"

#include <signal.h>

#ifdef USE_GC

// TODO: Using just the hlt_gc_* functions doesn't produce reliabley results
// currently, so we also configure the collector at compile time to simply
// replace all memory functions with its own versions. See
// libhilti/scripts/do-build. However, we should figure out at some point why
// that's necessary. Part of the problem is probably the 3rdparty code, which
// doesn't use the hlt_gc_* functions ...

// We install a signal handler for USR1 to dump force a collection and then
// dump the current GC state.
static void __hlt_gc_dump(int i)
{
    static int in_handler = 0;

    if ( in_handler )
        return;

    in_handler = 1;
    GC_gcollect();
#ifdef GC_DEBUG
    GC_generate_random_backtrace();
#endif
    GC_dump();
    in_handler = 0;
}

#ifdef GC_DEBUG
// In debug mode, if the environment variable HILTI_SAMPLE_HEAP is set to an
// integer value <n>, we dump the current GC state every <n> memory
// operations.
static int stack_sample_factor = 0;

void sample_heap()
{
    static uint64_t counter = 0;

    if ( ! stack_sample_factor )
        return;

    if ( ++counter % stack_sample_factor == 0 )
        __hlt_gc_dump(0);
}

// Likewise, in debug mode, we pass location information into the GC
// allocation functions.
#ifdef GC_ADD_CALLER
# define GC_PASS_EXTRA GC_RETURN_ADDR, file, line
#else
# define GC_PASS_EXTRA file, line
#endif

#endif // GC_DEBUG

void __hlt_gc_init()
{
    GC_INIT();

#ifdef GC_DEBUG
    char* env = getenv("HILTI_SAMPLE_HEAP");
    if ( env )
        stack_sample_factor = atoi(env);
#endif
    // Disabled for now.
    //
    // GC_enable_incremental();
    //
    signal(SIGUSR1, __hlt_gc_dump);
}

void __hlt_gc_force()
{
    GC_gcollect();
}

void* __hlt_gc_malloc_atomic(uint64_t n, const char* file, uint32_t line)
{
#ifndef GC_DEBUG
    return GC_malloc_atomic(n);
#else
    sample_heap();
    return GC_debug_malloc_atomic(n, GC_PASS_EXTRA);
#endif
}

void* __hlt_gc_malloc_non_atomic(uint64_t n, const char* file, uint32_t line)
{
#ifndef GC_DEBUG
    return GC_malloc(n);
#else
    sample_heap();
    return GC_debug_malloc(n, GC_PASS_EXTRA);
#endif
}

void* __hlt_gc_calloc_atomic(uint64_t count, uint64_t n, const char* file, uint32_t line)
{
#ifndef GC_DEBUG
    return GC_malloc_atomic(count * n);
#else
    sample_heap();
    return GC_debug_malloc_atomic(count * n, GC_PASS_EXTRA);
#endif
}

void* __hlt_gc_calloc_non_atomic(uint64_t count, uint64_t n, const char* file, uint32_t line)
{
#ifndef GC_DEBUG
    return GC_malloc(count * n);
#else
    sample_heap();
    return GC_debug_malloc(count * n, GC_PASS_EXTRA);
#endif
}

void* __hlt_gc_realloc_atomic(void* ptr, uint64_t n, const char* file, uint32_t line)
{
#ifndef GC_DEBUG
    return GC_realloc(ptr, n);
#else
    sample_heap();
    return GC_debug_realloc(ptr, n, GC_PASS_EXTRA);
#endif
}

void* __hlt_gc_realloc_non_atomic(void* ptr, uint64_t n, const char* file, uint32_t line)
{
#ifndef GC_DEBUG
    return GC_realloc(ptr, n);
#else
    sample_heap();
    return GC_debug_realloc(ptr, n, GC_PASS_EXTRA);
#endif
}

static void _finalizer(void* obj, void* client_data)
{
    (*((hlt_gc_finalizer_func*)client_data))(obj);
}

void hlt_gc_register_finalizer(void* obj, hlt_gc_finalizer_func* func)
{
    GC_REGISTER_FINALIZER(obj, _finalizer, func, 0, 0);
}

#else // not USE_GC. For testing, without GC support.

void __hlt_gc_init()
{
}

void __hlt_gc_force()
{
}

void* __hlt_gc_malloc_atomic(uint64_t n, const char* file, uint32_t line)
{
    return malloc(n);
}

void* __hlt_gc_malloc_non_atomic(uint64_t n, const char* file, uint32_t line)
{
    return malloc(n);
}

void* __hlt_gc_calloc_atomic(uint64_t count, uint64_t n, const char* file, uint32_t line)
{
    return calloc(count, n);
}

void* __hlt_gc_calloc_non_atomic(uint64_t count, uint64_t n, const char* file, uint32_t line)
{
    return calloc(count, n);
}

void* __hlt_gc_realloc_atomic(void* ptr, uint64_t n, const char* file, uint32_t line)
{
    return realloc(ptr, n);
}

void* __hlt_gc_realloc_non_atomic(void* ptr, uint64_t n, const char* file, uint32_t line)
{
    return realloc(ptr, n);
}

void hlt_gc_register_finalizer(void* obj, hlt_gc_finalizer_func* func)
{
    // Can't do finalizers.
}

#endif // USE_GC

void hlt_memory_clear(void* ptr, uint64_t size)
{
    memset(ptr, 0, (size_t)size);
}
