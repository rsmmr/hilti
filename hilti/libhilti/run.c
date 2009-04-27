/* $Id$
 *
 * Entry point from C to the HILTI framework. Handles setting up framework data structures, creating
 * threads, starting the schedulers, and entering the HILTI program itself.
 * 
 */

#include <stdio.h>
#include <signal.h>
#include <pthread.h>

#include "hilti_intern.h"

void hilti_multithreaded_run(__hlt_exception* hilti_except)
{
    // Read configuration.
    hilti_config config = hilti_config_get();

    // Initialize the exception variable.
    *hilti_except = 0;

    // If the user requested fewer than 2 threads, they must have written a program that does not
    // require a scheduler. We just run main_run() directly, without creating a thread context.
    if (config.num_threads < 2)
    {
        main_run(hilti_except);
        return;
    }

    // If we've reached this point, we need to create a HILTI thread context.
    // The thread context will create the threads specified in the configuration and
    // start them running automatically.
    __hlt_thread_context* context = __hlt_new_thread_context(&config);

    // Execute the HILTI main function. When it terminates, we return control to the caller.
    //main_run(hilti_except);
    __hlt_run_main_thread(context, main_run, hilti_except);

    // Did an exception occur?
    if (*hilti_except)
    {
        // When an exception occurs, we bring down all threads in the context immediately.
        __hlt_set_thread_context_state(context, __HLT_KILL);

        // Now we simply return as usual and give the caller a chance to handle the exception
        // as they see fit.
    }
    else
    {
        // Set the context's run state to STOP, which will allow existing jobs to finish but
        // prevent any new jobs from being scheduled. __hlt_set_run_state is synchronous
        // and will not return until all threads have terminated.
        __hlt_set_thread_context_state(context, __HLT_STOP);
    }

    // Now that all threads have terminated, destroy the thread context and return.
    __hlt_delete_thread_context(context);
}
