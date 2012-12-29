
#include <assert.h>
#include <string.h>

#include "context.h"
#include "globals.h"

static __hlt_global_state  our_globals;
static __hlt_global_state* globals = 0;

void __hlt_global_state_init()
{
    globals = &our_globals;

    memset(&our_globals, 0, sizeof(our_globals));

    __hlt_config_init();

    globals->context = __hlt_execution_context_new(HLT_VID_MAIN);

    __hlt_debug_init();
    __hlt_cmd_queue_init();
    __hlt_hooks_init();
    __hlt_threading_init();
    __hlt_profiler_init();
}

void __hlt_global_state_done()
{
    hlt_exception* excpt = 0;

    __hlt_threading_done(&excpt);
    __hlt_profiler_done(); // Must come after threading is done.

    if ( excpt ) {
        hlt_exception_print_uncaught(excpt, globals->context);
        GC_DTOR(excpt, hlt_exception);
    }

    __hlt_hooks_done();
    __hlt_cmd_queue_done();
    __hlt_debug_done();
    __hlt_config_done();

    GC_DTOR(globals->context, hlt_execution_context);

    if ( globals->debug_streams )
        free(globals->debug_streams);
}

__hlt_global_state* __hlt_globals()
{
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




