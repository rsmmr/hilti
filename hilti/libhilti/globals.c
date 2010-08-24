// $Id$

#include "hilti.h"

hlt_execution_context* __hlt_global_execution_context = 0;
hlt_thread_mgr*        __hlt_global_thread_mgr = 0;
hlt_hook_state*        __hlt_global_hook_state = 0;
pthread_mutex_t        __hlt_global_hook_state_lock;

hlt_execution_context* hlt_global_execution_context()
{
    assert(__hlt_global_execution_context);
    return __hlt_global_execution_context;
}

