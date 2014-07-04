
#include <assert.h>
#include <string.h>

#include "context.h"
#include "globals.h"
#include "config.h"

static __hlt_global_state  our_globals;
static __hlt_global_state* globals = 0;
static int globals_initialized = 0;

void __hlt_global_state_init(int init)
{
    if ( ! globals ) {
        globals = &our_globals;
        memset(&our_globals, 0, sizeof(our_globals));
        globals->config = __hlt_default_config();
    }

    if ( globals_initialized || ! init )
        return;

    globals->context = __hlt_execution_context_new_ref(HLT_VID_MAIN, 1);
    globals->multi_threaded = (__hlt_globals()->config->num_workers != 0);

    __hlt_debug_init();
    __hlt_fiber_init();
    __hlt_cmd_queue_init();
    __hlt_hooks_init();
    __hlt_threading_init();
    __hlt_profiler_init();

    globals_initialized = 1;
}

void __hlt_global_state_done()
{
    hlt_exception* excpt = 0;

    __hlt_threading_done(&excpt);
    __hlt_profiler_done(); // Must come after threading is done.

    if ( excpt ) {
        hlt_exception_print_uncaught(excpt, globals->context);
        GC_DTOR(excpt, hlt_exception, globals->context);
    }

    __hlt_hooks_done();
    __hlt_cmd_queue_done();
    __hlt_fiber_done();
    __hlt_debug_done();

    hlt_execution_context_delete(globals->context);

    if ( globals->debug_streams )
        free(globals->debug_streams);

    free(globals->config);
}

__hlt_global_state* __hlt_globals()
{
    if ( ! globals ) {
        __hlt_global_state_init(1);
        assert(globals);
    }

    return globals;
}

__hlt_global_state* __hlt_globals_object_no_init()
{
    if ( ! globals ) {
        __hlt_global_state_init(0);
        assert(globals);
    }

    return globals;
}

void __hlt_globals_set(__hlt_global_state* state)
{
    globals = state;
}

hlt_execution_context* hlt_global_execution_context()
{
    return globals->context;
}

hlt_thread_mgr* hlt_global_thread_mgr()
{
    return globals->thread_mgr;
}

void __hlt_global_set_thread_mgr(hlt_thread_mgr* mgr)
{
    globals->thread_mgr = mgr;
}




