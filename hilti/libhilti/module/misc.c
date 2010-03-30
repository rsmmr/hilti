// $Id$

#include <stdlib.h>

#include "module.h"

void hilti_abort(hlt_exception** excpt)
{
    abort();
}

// A non-yielding sleep.
void hilti_sleep(double secs, hlt_exception** excpt)
{
    struct timespec sleep_time;
    sleep_time.tv_sec = (time_t) secs;
    sleep_time.tv_nsec = (time_t) ((secs - sleep_time.tv_sec) * 1e9);
    
    while ( nanosleep(&sleep_time, &sleep_time) )
        continue;
}

void hilti_wait_for_threads()
{
    hlt_thread_mgr_set_state(__hlt_global_thread_mgr, HLT_THREAD_MGR_FINISH);
}
