#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <stdint.h>

#include "hilti.h"

///////////////////////////////////////////////////////////////////////////
// Data types related to worker threads.
///////////////////////////////////////////////////////////////////////////

// Typedefs used for interactions between C and HILTI.
typedef void (*__hlt_hilti_function)(void*);
typedef void* __hlt_hilti_frame;
typedef void (*__hlt_main_function)(__hlt_exception*);

// This enumeration lists the possible states for a thread context.
// RUN means that the worker threads should keep running, even if they run out
// of jobs.
// STOP means that the worker threads should disallow any new job scheduling,
// and terminate once their job queue is empty.
// KILL means that the worker threads should immediately terminate, regardless of
// the contents of their jobs queues.
// DEAD means that the worker threads are dead.
// Once could imagine another option such as STOP_WHEN_READY in which
// new jobs could still be scheduled, but the worker thread would
// terminate once its job queue becomes empty. This is not currently
// implemented, but might be useful or even preferable depending on the
// application.
// Currently, a thread context starts in the RUN state, and the allowable
// state transitions are:
// RUN -> STOP (transitions to DEAD automatically)
// RUN -> KILL (transitions to DEAD automatically)
// Since there is not an asynchronous version of __hlt_set_thread_context_state, (yet?)
// setting the state to STOP or KILL will always result in a state transition
// to DEAD by the time control to passed back to the caller. Note that it is illegal
// to delete a thread context which is not in the DEAD state. One could imagine adding
// the ability to transition back to RUN from the DEAD state in the future, so that
// a thread context could be reused.
typedef enum
{
    __HLT_RUN,
    __HLT_STOP,
    __HLT_KILL,
    __HLT_DEAD
} __hlt_thread_context_state;

// An enumeration used to distinguish between exceptions which have already been handled
// and exceptions which still need to be handled.
typedef enum
{
    __HLT_UNHANDLED,
    __HLT_HANDLED
} __hlt_exception_state;

struct __hlt_worker_thread_t;

// This struct encapsulates global state that all threads share.
typedef struct
{
    __hlt_thread_context_state state;
    long sleep_ns;
    unsigned watchdog_s;
    uint32_t num_worker_threads;
    struct __hlt_worker_thread_t* worker_threads;
    struct __hlt_worker_thread_t* main_thread;
    __hlt_main_function main_function;
} __hlt_thread_context;

// This struct represents a node in a job queue.
typedef struct __hlt_job_node_t
{
    __hlt_hilti_function function;
    __hlt_hilti_frame frame;
    struct __hlt_job_node_t* next;
} __hlt_job_node;

// This struct encapsulates the data related to a single worker thread.
typedef struct __hlt_worker_thread_t
{
    pthread_t thread_handle;
    uint32_t thread_id;
    pthread_mutex_t mutex;
    __hlt_thread_context* context;
    __hlt_job_node* job_queue_head;
    __hlt_job_node* job_queue_tail;
    __hlt_exception except;
    __hlt_exception_state except_state;
} __hlt_worker_thread;

///////////////////////////////////////////////////////////////////////////
// Thread context manipulation functions.
//
// These functions are designed for client code that wants to make use of
// HILTI thread contexts. Use these interfaces instead of manipulating the
// structs directly.
//
// Note that these functions are not reentrant; client code should call
// them from only one thread for a given context.
//
///////////////////////////////////////////////////////////////////////////

extern __hlt_thread_context* __hlt_new_thread_context(const hilti_config* config);
extern void __hlt_delete_thread_context(__hlt_thread_context* context);
extern void __hlt_set_thread_context_state(__hlt_thread_context* context, const __hlt_thread_context_state new_state);
extern __hlt_thread_context_state __hlt_get_thread_context_state(const __hlt_thread_context* context);
extern void __hlt_run_main_thread(__hlt_thread_context* context, __hlt_main_function function, __hlt_exception* except);
extern __hlt_exception __hlt_get_next_exception(__hlt_thread_context* context);

///////////////////////////////////////////////////////////////////////////
// Internal scheduling functions.
//
// Meant for use by the global scheduler and other interfaces between
// C and HILTI.
//
///////////////////////////////////////////////////////////////////////////

extern __hlt_thread_context* __hlt_get_current_thread_context();
extern uint32_t __hlt_get_current_thread_id();
extern uint32_t __hlt_get_thread_count(__hlt_thread_context* context);
extern void __hlt_schedule_job(__hlt_thread_context* context, uint32_t thread_id, __hlt_hilti_function function, __hlt_hilti_frame frame);

///////////////////////////////////////////////////////////////////////////
// External interfaces between C and HILTI.
///////////////////////////////////////////////////////////////////////////
extern void __hlt_call_hilti(__hlt_hilti_function function, __hlt_hilti_frame frame);

#endif
