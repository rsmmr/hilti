#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 199309

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "hilti.h"
#include "context.h"
#include "thread_context.h"

// FIXME: Should a thread context start in the RUN state? Should we be able to reuse a thread context?

// FIXME: Might be nice to provide a function to delete the thread specific data keys and reset the once variable.
// Only the calling code could know when it's safe to do that, since theoretically multiple thread contexts may exist.

// Forward declarations.
static hlt_job_node* hlt_new_job_node(hlt_hilti_function function, hlt_hilti_frame frame);
static void hlt_delete_job_node(hlt_job_node* job);
static void* hlt_main_scheduler(void* main_thread_ptr);
static void* hlt_worker_scheduler(void* worker_thread_ptr);

// Constants.
static const long __HLT_WAIT_FOR_SETSPECIFIC_NS = 10000000;

// Thread-specific data keys and related data.
static pthread_once_t hlt_once_control = PTHREAD_ONCE_INIT;
static pthread_key_t hlt_thread_context_key;
static pthread_key_t hlt_thread_id_key;

int8_t hlt_is_multi_threaded()
{
    return hilti_config_get()->num_threads >= 2;
}

static void hlt_init_pthread_keys()
{
    if (pthread_key_create(&hlt_thread_context_key, NULL) || pthread_key_create(&hlt_thread_id_key, NULL))
    {
        fputs("libhilti: cannot create pthread data keys.\n", stderr);
        exit(1);
    }
}

hlt_thread_context* hlt_new_thread_context(const hilti_config* config)
{
    // Do some one-time initialization if this is the first time that a thread
    // context has been created.
    pthread_once(&hlt_once_control, hlt_init_pthread_keys);

    // Create the context.
    hlt_thread_context* context = malloc(sizeof(hlt_thread_context));

    if (context == NULL)
    {
        fputs("libhilti: cannot allocate memory for thread context.\n", stderr);
        exit(1);
    }

    // Initialize its values based upon the configuration.
    context->state = HLT_RUN;

    context->sleep_ns = config->sleep_ns;
    context->watchdog_s = config->watchdog_s;
    context->stack_size = config->stack_size;

    if (config->num_threads == 0)
        context->num_worker_threads = 1;
    else
        context->num_worker_threads = config->num_threads;

    context->worker_threads = malloc(sizeof(hlt_worker_thread) * context->num_worker_threads);

    if (context->worker_threads == NULL)
    {
        fputs("libhilti: cannot allocate memory for worker threads.\n", stderr);
        exit(1);
    }

    // Set up the thread attributes.
    pthread_attr_init(&context->thread_attributes);
    pthread_attr_setstacksize(&context->thread_attributes, context->stack_size);

    // Initialize the worker threads it contains.
    for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
    {
        context->worker_threads[i].context = context;
        context->worker_threads[i].thread_id = i;
        context->worker_threads[i].job_queue_head = NULL;
        context->worker_threads[i].job_queue_tail = NULL;
        context->worker_threads[i].except = NULL;
        context->worker_threads[i].except_state = HLT_HANDLED;
        
        __hlt_execution_context_init(&context->worker_threads[i].exec_context);

        pthread_mutex_init(&context->worker_threads[i].mutex, NULL);
       
        if (pthread_create(&context->worker_threads[i].thread_handle, &context->thread_attributes, hlt_worker_scheduler, (void*) &context->worker_threads[i]))
        {
            fputs("libhilti: cannot create worker threads.\n", stderr);
            exit(1);
        }
    }

    // Create the 'main thread' struct. The main thread is actually a fake thread that just redirects all
    // scheduling to the first worker thread. It only exists to allow HILTI main functions to exist
    // within a thread context and do things like yield and schedule other threads.
    context->main_thread = malloc(sizeof(hlt_worker_thread));
    context->main_thread->thread_id = 0;
    context->main_thread->context = context;
    context->main_thread->job_queue_head = NULL;
    context->main_thread->job_queue_tail = NULL;
    context->main_thread->except = NULL;
    context->main_thread->except_state = HLT_HANDLED;
    
    __hlt_execution_context_init(&context->main_thread->exec_context);

    // Initially there is no main function running.
    context->main_function = NULL;

    return context;
}

void hlt_delete_thread_context(hlt_thread_context* context)
{
    // Calling this function is an error if threads in this context are still running.
    if (context->state != HLT_DEAD)
    {
        fputs("libhilti: the thread context being deleted still has running threads.\n", stderr);
        exit(1);
    }

    if (context->main_function != NULL)
    {
        fputs("libhilti: cannot delete a thread context that has a main thread running.\n", stderr);
        exit(1);
    }

    // Destroy the thread attributes object.
    pthread_attr_destroy(&context->thread_attributes);

    // Free the memory associated with the job queue and destroy other internal thread structures.
    for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
    {
        if (pthread_mutex_destroy(&context->worker_threads[i].mutex))
        {
            fputs("libhilti: could not destroy mutex.\n", stderr);
            exit(1);
        }

        hlt_job_node* current = context->worker_threads[i].job_queue_head;

        while (current != NULL)
        {
            hlt_job_node* next = current->next;
            hlt_delete_job_node(current);
            current = next;
        }
        
        __hlt_execution_context_done(&context->worker_threads[i].exec_context);
    }

    __hlt_execution_context_done(&context->main_thread->exec_context);
    
    // Free the worker thread structures.
    free(context->worker_threads);

    // Free the main thread structures.
    free(context->main_thread);

    // Free the context.
    free(context);
}

static void* hlt_watchdog(void* context_ptr)
{
    hlt_thread_context* context = (hlt_thread_context*) context_ptr;

    // Prepare the nanosleep data structure.
    struct timespec sleep_time;
    sleep_time.tv_sec = (time_t) context->watchdog_s;
    sleep_time.tv_nsec = 0;

    // Sleep. We'll be cancelled during this time if we're no longer
    // needed.
    while (nanosleep(&sleep_time, &sleep_time))
        continue;

    // Double check that we haven't been canceled.
    pthread_testcancel();

    // Cancel all worker threads. No error reporting since some of these
    // threads may legitimately no longer exist.
    for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
        pthread_cancel(context->worker_threads[i].thread_handle);

    fputs("libhilti: warning: watchdog had to kill threads.\n", stderr);

    return NULL;
}

void hlt_set_thread_context_state(hlt_thread_context* context, const hlt_thread_context_state new_state)
{
    // If we're not making a change, do nothing.
    if (context->state == new_state)
        return;

    if (context->state == HLT_RUN && new_state == HLT_STOP)
    {
        // Acquire every thread's lock. We do this to ensure that there are no partially scheduled
        // jobs before we change the state of the thread context.
        for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
            pthread_mutex_lock(&context->worker_threads[i].mutex);

        {
            // Update the state to stop any new jobs from being scheduled.
            context->state = HLT_STOP;
        }

        // Release every thread's lock.
        for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
            pthread_mutex_unlock(&context->worker_threads[i].mutex);

        // Start watchdog timer.
        pthread_t watchdog_handle;
        if (pthread_create(&watchdog_handle, NULL, hlt_watchdog, (void*) context))
        {
            fputs("libhilti: cannot create watchdog timer for thread join.\n", stderr);
            exit(1);
        }
 
        // Join the threads.
        for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
        {
            if (pthread_join(context->worker_threads[i].thread_handle, NULL))
            {
                fputs("libhilti: error joining worker threads.\n", stderr);
                exit(1);
            }
        }

        // Cancel the watchdog timer. No error reporting since the watchdog timer
        // thread may have terminated legitimately by now.
        pthread_cancel(watchdog_handle);

        // Update the run state to reflect the fact that the threads are dead.
        context->state = HLT_DEAD;
    }
    else if (context->state == HLT_RUN && new_state == HLT_KILL)
    {
        // Acquire every thread's lock. We do this to ensure that there are no partially scheduled
        // jobs before we change the state of the thread context.
        for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
            pthread_mutex_lock(&context->worker_threads[i].mutex);

        {
            // Update the state to stop any new jobs from being scheduled.
            context->state = HLT_KILL;
        }

        // Release every thread's lock.
        for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
            pthread_mutex_unlock(&context->worker_threads[i].mutex);

        // Cancel the threads.
        for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
        {
            if (pthread_cancel(context->worker_threads[i].thread_handle))
            {
                fputs("libhilti: error cancelling worker threads.\n", stderr);
                exit(1);
            }
        }

        // Ensure they've actually died.
        for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
            pthread_join(context->worker_threads[i].thread_handle, NULL);

        // Update the run state to reflect the fact that the threads are dead.
        context->state = HLT_DEAD;
    }
    else
    {
        fputs("libhilti: warning: illegal run state transition attempted.\n", stderr);
    }
}

hlt_thread_context_state hlt_get_thread_context_state(const hlt_thread_context* context)
{
    return context->state;
}


hlt_exception* hlt_get_next_exception(hlt_thread_context* context)
{
    // Calling this function is an error if threads in this context are still running.
    if (context->state != HLT_DEAD)
    {
        fputs("libhilti: the thread context being checked for exceptions still has running threads.\n", stderr);
        exit(1);
    }

    if (context->main_function != NULL)
    {
        fputs("libhilti: cannot check a thread context for exceptions when a main thread is still running.\n", stderr);
        exit(1);
    }

    // Locate the first exception that hasn't been reported already, remove it,
    // and return it to the user.
    for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
    {
        if (context->worker_threads[i].except != NULL)
        {
            hlt_exception* except = context->worker_threads[i].except;
            context->worker_threads[i].except = NULL;
            return except;
        }
    }

    // There are no exceptions that haven't already been reported, so return NULL.
    return NULL;
}

void hlt_run_main_thread(hlt_thread_context* context, hlt_main_function function, hlt_exception** except)
{
    if (context->state != HLT_RUN)
    {
        fputs("libhilti: cannot run main thread on a thread context that is not in the RUN state.\n", stderr);
        exit(1);
    }

    if (context->main_function != NULL)
    {
        fputs("libhilti: cannot run main thread on a thread context that already has a main thread running.\n", stderr);
        exit(1);
    }

    // Store the main function in the thread context so that the main scheduler can access it.
    context->main_function = function;

    // Create the actual thread. Its scheduler does only one thing: run the function it has been
    // passed, and store any exceptions that result.
    if (pthread_create(&context->main_thread->thread_handle, &context->thread_attributes, hlt_main_scheduler, (void*) context->main_thread))
    {
        fputs("libhilti: cannot create main thread.\n", stderr);
        exit(1);
    }

    // Wait for the main thread to complete. This provides the synchronous behavior the user expects.
    if (pthread_join(context->main_thread->thread_handle, NULL))
    {
        fputs("libhilti: error joining main thread.\n", stderr);
        exit(1);
    }

    // Transmit any exception to the user.
    *except = context->main_thread->except;

    // Indicate that the main thread isn't running anymore.
    context->main_function = NULL;
}

static hlt_job_node* hlt_new_job_node(hlt_hilti_function function, hlt_hilti_frame frame)
{
    hlt_job_node* new_node = malloc(sizeof(hlt_job_node));

    new_node->function = function;
    new_node->frame = frame;
    new_node->next = NULL;

    return new_node;
}

static void hlt_delete_job_node(hlt_job_node* job)
{
    free(job);
}

static void* hlt_main_scheduler(void* main_thread_ptr)
{
    hlt_worker_thread* this_thread = (hlt_worker_thread*) main_thread_ptr;

    // Set up thread-specific data.
    if (pthread_setspecific(hlt_thread_context_key, (const void*) this_thread->context) || pthread_setspecific(hlt_thread_id_key, (const void*) &this_thread->thread_id))
    {
        fputs("libhilti: could not set specific pthread data.\n", stderr);
        exit(1);
    }

    // Execute the main function.
    this_thread->context->main_function(&this_thread->except);

    // Terminate the thread.
    return NULL;
}

static void* hlt_worker_scheduler(void* worker_thread_ptr)
{
    hlt_worker_thread* this_thread = (hlt_worker_thread*) worker_thread_ptr;
    hlt_job_node* job;

    // Set up thread-specific data.
    if (pthread_setspecific(hlt_thread_context_key, (const void*) this_thread->context) || pthread_setspecific(hlt_thread_id_key, (const void*) &this_thread->thread_id))
    {
        fputs("libhilti: could not set specific pthread data.\n", stderr);
        exit(1);
    }

    // Prepare the nanosleep data structure.
    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = this_thread->context->sleep_ns;

    while (1)
    {
        pthread_testcancel();

        // Sleep if we have nothing to do. (Terminating if asked to.)
        while (this_thread->job_queue_head == NULL)
        {
            if (this_thread->context->state == HLT_STOP)
            {
                // Prevent a race condition.

                // We acquire the lock here to make sure that any jobs that may be in the process
                // of being added complete that process. We only need to worry about jobs which
                // began the process of being added before the thread context state changed to
                // HLT_STOP. After acquiring and releasing this lock, any further attempts to
                // add jobs will definitely see that the current state is HLT_STOP and fail.
                pthread_mutex_lock(&this_thread->mutex);
                pthread_mutex_unlock(&this_thread->mutex);

                // If the job queue is empty at this point, we should return. Other, we need to
                // process the remaining jobs in the queue.
                if (this_thread->job_queue_head == NULL)
                    return NULL;
                else
                    break;
            }
            else
            {
                nanosleep(&sleep_time, NULL);
            }

            pthread_testcancel();
        }

        // Remove a single item from the job queue.
        pthread_mutex_lock(&this_thread->mutex);
        {
            job = this_thread->job_queue_head;

            // Make the next job the head of the queue.
            this_thread->job_queue_head = job->next;

            // If this was the only job in the queue (the head and tail
            // were the same) then the head is now NULL; we need to make
            // the tail NULL too.
            if (this_thread->job_queue_head == NULL)
                this_thread->job_queue_tail = NULL;
        }
        pthread_mutex_unlock(&this_thread->mutex);

        // Execute the job.
        hlt_call_hilti(job->function, job->frame);

        // Check for an unhandled exception.
        if (this_thread->except_state == HLT_UNHANDLED)
        {
            if (this_thread->context->main_function != NULL)
            {
                // If a main thread is running, (it will  be unless execution has already
                // completed successfully) cancel the thread and set its exception property
                // to the predefined WorkerThreadThrew exception.  After the main thread has
                // been canceled, hlt_run_main_thread() will return to the user and they
                // can take the action they deem appropriate. (Almost certainly, that means
                // setting the thread context state to KILL.)
                hlt_set_exception(&this_thread->context->main_thread->except, &hlt_exception_worker_thread_threw_exception, 0);

                if (pthread_cancel(this_thread->context->main_thread->thread_handle))
                {
                    fputs("libhilti: error cancelling main thread after exception.\n", stderr);
                    exit(1);
                }
            }

            // We've now handled this exception.
            this_thread->except_state = HLT_HANDLED;

            // We keep running until the user kills us. This is simpler and provides uniform
            // semantics no matter which thread experiences an exception.
        }

        // Free the memory associated with the job.
        hlt_delete_job_node(job);
    }

    return NULL;
}

hlt_thread_context* __hlt_get_current_thread_context()
{
     hlt_thread_context* context = (hlt_thread_context*) pthread_getspecific(hlt_thread_context_key);

     return context;
}

uint32_t __hlt_get_current_thread_id()
{
    uint32_t* thread_id = (uint32_t*) pthread_getspecific(hlt_thread_id_key);

    return *thread_id;
}

uint32_t __hlt_get_thread_count(hlt_thread_context* context)
{
    return context->num_worker_threads;
}

void __hlt_schedule_job(hlt_thread_context* context, uint32_t thread_id, hlt_hilti_function function, hlt_hilti_frame frame)
{
    hlt_worker_thread* target_thread = &context->worker_threads[thread_id];

    pthread_mutex_lock(&target_thread->mutex);
    {
        // Only schedule a job if we're in the RUN state.
        if (context->state == HLT_RUN)
        {
            hlt_job_node* job = hlt_new_job_node(function, frame);

            if (target_thread->job_queue_tail == NULL)
            {
                // There are no jobs currently scheduled on this thread.
                target_thread->job_queue_head = job;
                target_thread->job_queue_tail = job;
            }
            else
            {
                // Schedule this job at the end of the job queue.
                target_thread->job_queue_tail->next = job;
                target_thread->job_queue_tail = job;
            }
        }
    }
    pthread_mutex_unlock(&target_thread->mutex);
}

void hlt_report_local_exception(hlt_exception* except)
{
    hlt_thread_context* context = __hlt_get_current_thread_context();
    uint32_t thread_id = __hlt_get_current_thread_id();
    hlt_worker_thread* target_thread = &context->worker_threads[thread_id];

    // Notify the worker thread that an exception has occurred.
    target_thread->except = except;
    target_thread->except_state = HLT_UNHANDLED;
}
