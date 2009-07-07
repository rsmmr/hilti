#ifndef HILTI_WORKER_THREAD_H
#define HILTI_WORKER_THREAD_H

#include <stdint.h>

#include "exceptions.h"

///////////////////////////////////////////////////////////////////////////
// Data types related to worker threads.
///////////////////////////////////////////////////////////////////////////

// Typedefs used for interactions between C and HILTI.
typedef void (*hlt_hilti_function)(void*);
typedef void* hlt_hilti_frame;
typedef void (*hlt_main_function)(hlt_exception*);

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
// Since there is not an asynchronous version of hlt_set_thread_context_state, (yet?)
// setting the state to STOP or KILL will always result in a state transition
// to DEAD by the time control to passed back to the caller. Note that it is illegal
// to delete a thread context which is not in the DEAD state. One could imagine adding
// the ability to transition back to RUN from the DEAD state in the future, so that
// a thread context could be reused.
typedef enum
{
    HLT_RUN,
    HLT_STOP,
    HLT_KILL,
    HLT_DEAD
} hlt_thread_context_state;

// An enumeration used to distinguish between exceptions which have already been handled
// and exceptions which still need to be handled.
typedef enum
{
    HLT_UNHANDLED,
    HLT_HANDLED
} hlt_exception_state;

struct hlt_worker_thread_t;

// This struct encapsulates global state that all threads share.
typedef struct
{
    hlt_thread_context_state state;
    long sleep_ns;
    unsigned watchdog_s;
    size_t stack_size;
    pthread_attr_t thread_attributes;
    uint32_t num_worker_threads;
    struct hlt_worker_thread_t* worker_threads;
    struct hlt_worker_thread_t* main_thread;
    hlt_main_function main_function;
} hlt_thread_context;

// This struct represents a node in a job queue.
typedef struct hlt_job_node_t
{
    hlt_hilti_function function;
    hlt_hilti_frame frame;
    struct hlt_job_node_t* next;
} hlt_job_node;

// This struct encapsulates the data related to a single worker thread.
typedef struct hlt_worker_thread_t
{
    pthread_t thread_handle;
    uint32_t thread_id;
    pthread_mutex_t mutex;
    hlt_thread_context* context;
    hlt_job_node* job_queue_head;
    hlt_job_node* job_queue_tail;
    hlt_exception except;
    hlt_exception_state except_state;
} hlt_worker_thread;

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

extern hlt_thread_context* hlt_new_thread_context(const hilti_config* config);
extern void hlt_delete_thread_context(hlt_thread_context* context);
extern void hlt_set_thread_context_state(hlt_thread_context* context, const hlt_thread_context_state new_state);
extern hlt_thread_context_state hlt_get_thread_context_state(const hlt_thread_context* context);
extern void hlt_run_main_thread(hlt_thread_context* context, hlt_main_function function, hlt_exception* except);
extern hlt_exception hlt_get_next_exception(hlt_thread_context* context);

///////////////////////////////////////////////////////////////////////////
// Internal scheduling functions.
//
// Meant for use by the global scheduler and other interfaces between
// C and HILTI.
//
///////////////////////////////////////////////////////////////////////////

extern hlt_thread_context* __hlt_get_current_thread_context();
extern uint32_t __hlt_get_current_thread_id();
extern uint32_t __hlt_get_thread_count(hlt_thread_context* context);
extern void __hlt_schedule_job(hlt_thread_context* context, uint32_t thread_id, hlt_hilti_function function, hlt_hilti_frame frame);

///////////////////////////////////////////////////////////////////////////
// External interfaces between C and HILTI.
///////////////////////////////////////////////////////////////////////////
extern void hlt_call_hilti(hlt_hilti_function function, hlt_hilti_frame frame);

#endif
