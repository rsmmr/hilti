
#include <stdlib.h>
#include <ctype.h>

#include "module.h"
#include "../globals.h"

void hilti_abort(hlt_exception** excpt)
{
    fprintf(stderr, "Hilti::abort() called.");
    abort();
}

void hilti_terminate(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set_exception(excpt, &hlt_exception_termination, 0);
}

// A non-yielding sleep.
void hilti_sleep(double secs, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_thread_mgr* mgr = hlt_global_thread_mgr();

    // Flush our job queues so that the stuff can get processes.
    for ( int i = 0; i < mgr->num_workers; ++i )
        hlt_thread_queue_flush(mgr->workers[i]->jobs, 0);

    struct timespec sleep_time;
    sleep_time.tv_sec = (time_t) secs;
    sleep_time.tv_nsec = (time_t) ((secs - sleep_time.tv_sec) * 1e9);

    while ( nanosleep(&sleep_time, &sleep_time) )
        continue;
}

void hilti_wait_for_threads()
{
    hlt_thread_mgr_set_state(hlt_global_thread_mgr(), HLT_THREAD_MGR_FINISH);
}

