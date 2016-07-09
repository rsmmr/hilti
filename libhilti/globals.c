
#include <assert.h>
#include <inttypes.h>
#include <string.h>

#include "cmdqueue.h"
#include "config.h"
#include "context.h"
#include "debug.h"
#include "fiber.h"
#include "globals.h"
#include "linker.h"
#include "memory.h"
#include "profiler.h"
#include "stackmap.h"
#include "threading.h"

static __hlt_global_state __our_globals;
static __hlt_global_state* __globals = &__our_globals;

static void __hlt_global_create_config()
{
    if ( ! __globals->config )
        __globals->config = __hlt_default_config();
}

void* __hlt_runtime_state_get()
{
    return __globals;
}

void __hlt_runtime_state_set(void* state)
{
    __globals = (__hlt_global_state*)state;
}

int __hlt_global_state_init()
{
    if ( __globals->initialized )
        return 0;

    __hlt_global_create_config();

    __globals->initialized = 1;
    __globals->globals_size = __hlt_globals_size();
    __globals->context = __hlt_execution_context_new_ref(HLT_VID_MAIN, 1);
    __globals->multi_threaded = (__globals->config->num_workers != 0);

    __hlt_debug_init();
    __hlt_fiber_init();
    __hlt_cmd_queue_init();
    __hlt_hooks_init();
    __hlt_threading_init();
    __hlt_profiler_init();
    __hlt_stackmap_init();

    return 1;
}

int __hlt_global_state_done()
{
    if ( ! __globals->initialized )
        return 0;

    if ( __globals->finished )
        return 0;

    __globals->finished = 1;

    hlt_exception* excpt = 0;

    __hlt_stackmap_done();
    __hlt_threading_done(&excpt);
    __hlt_profiler_done(); // Must come after threading is done.

    if ( excpt ) {
        hlt_exception_print_uncaught(excpt, __globals->context);
        GC_DTOR(excpt, hlt_exception, __globals->context);
    }

    __hlt_hooks_done();
    __hlt_cmd_queue_done();
    __hlt_fiber_done();
    __hlt_debug_done();

    hlt_execution_context_delete(__globals->context);

    if ( __globals->debug_streams )
        free(__globals->debug_streams);

    free(__globals->config);

    return 1;
}

__hlt_global_state* __hlt_globals()
{
    assert(__globals->initialized);
    return __globals;
}

// Internal use only.
__hlt_global_state* __hlt_globals_object_no_init()
{
    __hlt_global_create_config();
    return __globals;
}

hlt_execution_context* hlt_global_execution_context()
{
    return __globals->context;
}

hlt_thread_mgr* hlt_global_thread_mgr()
{
    return __globals->thread_mgr;
}

void __hlt_global_set_thread_mgr(hlt_thread_mgr* mgr)
{
    __globals->thread_mgr = mgr;
}

void hlt_global_state_dump(FILE* f)
{
    fprintf(f, "=== libhilti global state (at %p)\n", __globals);
    fprintf(f, "initialized : %s\n", (__globals->initialized ? "yes" : "no"));
    fprintf(f, "finished:     %s\n", (__globals->finished ? "yes" : "no"));
    fprintf(f, "globals_size: %" PRIu64 "\n", __globals->globals_size);
    fprintf(f, "context:      %p\n", __globals->context);
    fprintf(f, "debug:        %s\n", __globals->debug_streams);
    fprintf(f, "profiling:    %s\n", (__globals->profiling_enabled ? "yes" : "no"));
    fprintf(f, "papi:         %s\n", (__globals->papi_available ? "yes" : "no"));

    hlt_config_dump(f);
}
