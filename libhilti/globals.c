
#include <assert.h>

#include "context.h"
#include "globals.h"

// Note: Global variables should be limited to a mininum, and must be ensured
// that they are used in a thread-safe fashion.

static hlt_execution_context* _global_execution_context = 0;
static hlt_thread_mgr*        _global_thread_mgr = 0;

#if 0
static hlt_hook_state*        _global_hook_state = 0;
static pthread_mutex_t        _global_hook_state_lock;
#endif

void __hlt_global_state_init()
{
    _global_execution_context = __hlt_execution_context_new(HLT_VID_MAIN);
}

void __hlt_global_state_done()
{
    GC_DTOR(_global_execution_context, hlt_execution_context);
    _global_execution_context = 0;
}

hlt_execution_context* hlt_global_execution_context()
{
    assert(_global_execution_context);
    return _global_execution_context;
}

hlt_thread_mgr* hlt_global_thread_mgr()
{
    return _global_thread_mgr;
}




