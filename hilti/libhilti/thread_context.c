#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 199309

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "hilti_intern.h"
#include "thread_context.h"

// FIXME: Right now there's no handling of HILTI exceptions except for the main function.

// FIXME: Should a thread context start in the RUN state? Should we be able to reuse a thread context?

// Forward declarations.
static __hlt_job_node* __hlt_new_job_node(__hlt_hilti_function function, __hlt_hilti_continuation continuation);
static void __hlt_delete_job_node(__hlt_job_node* job);
static void* __hlt_main_scheduler(void* main_thread_ptr);
static void* __hlt_worker_scheduler(void* worker_thread_ptr);

// Constants.
static const long __HLT_WAIT_FOR_SETSPECIFIC_NS = 10000000;

// Thread-specific data keys and related data.
static pthread_once_t __hlt_once_control = PTHREAD_ONCE_INIT;
static pthread_key_t __hlt_thread_context_key;
static pthread_key_t __hlt_thread_id_key;

static void __hlt_init_pthread_keys()
{
    if (pthread_key_create(&__hlt_thread_context_key, NULL) || pthread_key_create(&__hlt_thread_id_key, NULL))
    {
        fputs("libhilti: cannot create pthread data keys.\n", stderr);
        exit(1);
    }
}

__hlt_thread_context* __hlt_new_thread_context(const hilti_config* config)
{
    // Do some one-time initialization if this is the first time that a thread
    // context has been created.
    pthread_once(&__hlt_once_control, __hlt_init_pthread_keys);

    // Create the context.
    __hlt_thread_context* context = malloc(sizeof(__hlt_thread_context));

    if (context == NULL)
    {
        fputs("libhilti: cannot allocate memory for thread context.\n", stderr);
        exit(1);
    }

    // Initialize its values based upon the configuration.
    context->state = __HLT_RUN;

    context->sleep_ns = config->sleep_ns;
    context->watchdog_s = config->watchdog_s;

    if (config->num_threads == 0)
        context->num_worker_threads = 1;
    else
        context->num_worker_threads = config->num_threads;

    context->worker_threads = malloc(sizeof(__hlt_worker_thread) * context->num_worker_threads);

    if (context->worker_threads == NULL)
    {
        fputs("libhilti: cannot allocate memory for worker threads.\n", stderr);
        exit(1);
    }

    // Initialize the worker threads it contains.
    for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
    {
        context->worker_threads[i].context = context;
        context->worker_threads[i].thread_id = i;
        context->worker_threads[i].job_queue_head = NULL;
        context->worker_threads[i].job_queue_tail = NULL;
        context->worker_threads[i].except = 0;

        pthread_mutex_init(&context->worker_threads[i].mutex, NULL);
       
        if (pthread_create(&context->worker_threads[i].thread_handle, NULL, __hlt_worker_scheduler, (void*) &context->worker_threads[i]))
        {
            fputs("libhilti: cannot create worker threads.\n", stderr);
            exit(1);
        }
    }

    // Initially, there is no main thread.
    context->main_thread = NULL;
    context->main_function = NULL;

    return context;
}

void __hlt_delete_thread_context(__hlt_thread_context* context)
{
    // Calling this function is an error if threads in this context are still running.
    if (context->state != __HLT_DEAD)
    {
        fputs("libhilti: the thread context being deleted still has running threads.\n", stderr);
        exit(1);
    }

    if (context->main_thread != NULL)
    {
        fputs("libhilti: cannot delete a thread context that has a main thread running.\n", stderr);
        exit(1);
    }

    // Free the memory associated with the job queue and destroy other internal thread structures.
    for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
    {
        if (pthread_mutex_destroy(&context->worker_threads[i].mutex))
        {
            fputs("libhilti: could not destroy mutex.\n", stderr);
            exit(1);
        }

        __hlt_job_node* current = context->worker_threads[i].job_queue_head;

        while (current != NULL)
        {
            __hlt_job_node* next = current->next;
            __hlt_delete_job_node(current);
            current = next;
        }
    }

    // Free the worker thread structures.
    free(context->worker_threads);

    // Free the context.
    free(context);
}

static void* __hlt_watchdog(void* context_ptr)
{
    __hlt_thread_context* context = (__hlt_thread_context*) context_ptr;

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

    return NULL;
}

void __hlt_set_thread_context_state(__hlt_thread_context* context, const __hlt_thread_context_state new_state)
{
    // If we're not making a change, do nothing.
    if (context->state == new_state)
        return;

    if (context->state == __HLT_RUN && new_state == __HLT_STOP)
    {
        // Acquire every thread's lock. We do this to ensure that there are no partially scheduled
        // jobs before we change the state of the thread context.
        for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
            pthread_mutex_lock(&context->worker_threads[i].mutex);

        {
            // Update the state to stop any new jobs from being scheduled.
            context->state = __HLT_STOP;
        }

        // Release every thread's lock.
        for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
            pthread_mutex_unlock(&context->worker_threads[i].mutex);

        // Start watchdog timer.
        pthread_t watchdog_handle;
        if (pthread_create(&watchdog_handle, NULL, __hlt_watchdog, (void*) context))
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
        context->state = __HLT_DEAD;
    }
    else if (context->state == __HLT_RUN && new_state == __HLT_KILL)
    {
        // Acquire every thread's lock. We do this to ensure that there are no partially scheduled
        // jobs before we change the state of the thread context.
        for (uint32_t i = 0 ; i < context->num_worker_threads ; i++)
            pthread_mutex_lock(&context->worker_threads[i].mutex);

        {
            // Update the state to stop any new jobs from being scheduled.
            context->state = __HLT_KILL;
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
        context->state = __HLT_DEAD;
    }
    else
    {
        fputs("libhilti: warning: illegal run state transition attempted.\n", stderr);
    }
}

__hlt_thread_context_state __hlt_get_thread_context_state(const __hlt_thread_context* context)
{
    return context->state;
}


void __hlt_run_main_thread(__hlt_thread_context* context, __hlt_main_function function, __hlt_exception* except)
{
    if (context->state != __HLT_RUN)
    {
        fputs("libhilti: cannot run main thread on a thread context that is not in the RUN state.\n", stderr);
        exit(1);
    }

    if (context->main_thread != NULL)
    {
        fputs("libhilti: cannot run main thread on a thread context that already has a main thread running.\n", stderr);
        exit(1);
    }

    // Create the 'main thread' struct. The main thread is actually a fake thread that just redirects all
    // scheduling to the first worker thread. It only exists to allow HILTI main functions to exist
    // within a thread context and do things like yield and schedule other threads.
    context->main_thread = malloc(sizeof(__hlt_worker_thread));
    context->main_thread->thread_id = 0;
    context->main_thread->context = context;
    context->main_thread->job_queue_head = NULL;
    context->main_thread->job_queue_tail = NULL;
    context->main_thread->except = 0;

    // Store the main function in the thread context so that the main scheduler can access it.
    context->main_function = function;

    // Create the actual thread. Its scheduler does only one thing: run the function it has been
    // passed, and store any exceptions that result.
    if (pthread_create(&context->main_thread->thread_handle, NULL, __hlt_main_scheduler, (void*) context->main_thread))
    {
        fputs("libhilti: cannot create main thread.\n", stderr);
        exit(1);
    }

    // Wait for the main thread to complete. This provides the synchronous behavior the user expects.
    if (pthread_join(context->main_thread->thread_handle, NULL))
    {
        fputs("libhilti: error joining main thread threads.\n", stderr);
        exit(1);
    }

    // Transmit any exception to the user.
    *except = context->main_thread->except;

    // Free the thread data structure, which we're done with.
    free(context->main_thread);
    context->main_thread = NULL;
    context->main_function = NULL;
}

static __hlt_job_node* __hlt_new_job_node(__hlt_hilti_function function, __hlt_hilti_continuation continuation)
{
    __hlt_job_node* new_node = malloc(sizeof(__hlt_job_node));

    new_node->function = function;
    new_node->continuation = continuation;
    new_node->next = NULL;

    return new_node;
}

static void __hlt_delete_job_node(__hlt_job_node* job)
{
    free(job);
}

static void* __hlt_main_scheduler(void* main_thread_ptr)
{
    __hlt_worker_thread* this_thread = (__hlt_worker_thread*) main_thread_ptr;

    // Set up thread-specific data.
    if (pthread_setspecific(__hlt_thread_context_key, (const void*) this_thread->context) || pthread_setspecific(__hlt_thread_id_key, (const void*) &this_thread->thread_id))
    {
        fputs("libhilti: could not set specific pthread data.\n", stderr);
        exit(1);
    }

    // Execute the main function.
    this_thread->context->main_function(&this_thread->except);

    // Terminate the thread.
    return NULL;
}

static void* __hlt_worker_scheduler(void* worker_thread_ptr)
{
    __hlt_worker_thread* this_thread = (__hlt_worker_thread*) worker_thread_ptr;
    __hlt_job_node* job;

    // Set up thread-specific data.
    if (pthread_setspecific(__hlt_thread_context_key, (const void*) this_thread->context) || pthread_setspecific(__hlt_thread_id_key, (const void*) &this_thread->thread_id))
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
        // Sleep if we have nothing to do. (Terminating if asked to.)
        while (this_thread->job_queue_head == NULL)
        {
            if (this_thread->context->state == __HLT_STOP)
                return NULL;
            else
                nanosleep(&sleep_time, NULL);
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
            if (this_thread->job_queue_tail == job)
                this_thread->job_queue_tail = NULL;
        }
        pthread_mutex_unlock(&this_thread->mutex);

        // Execute the job.
        __hlt_call_hilti(job->function, job->continuation);

        // Free the memory associated with the job.
        __hlt_delete_job_node(job);
    }

    return NULL;
}

__hlt_thread_context* __hlt_get_current_thread_context()
{
     __hlt_thread_context* context = (__hlt_thread_context*) pthread_getspecific(__hlt_thread_context_key);

     while (context == NULL)
     {
        // pthread_setspecific hasn't been called yet. We've just got to wait it out.
        struct timespec sleep_time;
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = __HLT_WAIT_FOR_SETSPECIFIC_NS;

        nanosleep(&sleep_time, NULL);
        
        context = (__hlt_thread_context*) pthread_getspecific(__hlt_thread_context_key);
     }

     return context;
}

uint32_t __hlt_get_current_thread_id()
{
    uint32_t* thread_id = (uint32_t*) pthread_getspecific(__hlt_thread_id_key);

    while (thread_id == NULL)
    {
        // pthread_setspecific hasn't been called yet. We've just got to wait it out.
        struct timespec sleep_time;
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = __HLT_WAIT_FOR_SETSPECIFIC_NS;

        nanosleep(&sleep_time, NULL);

        thread_id = (uint32_t*) pthread_getspecific(__hlt_thread_id_key);
    }

    return *thread_id;
}

uint32_t __hlt_get_thread_count(__hlt_thread_context* context)
{
    return context->num_worker_threads;
}

void __hlt_schedule_job(__hlt_thread_context* context, uint32_t thread_id, __hlt_hilti_function function, __hlt_hilti_continuation continuation)
{
    __hlt_worker_thread* target_thread = &context->worker_threads[thread_id];

    pthread_mutex_lock(&target_thread->mutex);
    {
        // Only schedule a job if we're in the RUN state.
        if (context->state == __HLT_RUN)
        {
            __hlt_job_node* job = __hlt_new_job_node(function, continuation);

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
