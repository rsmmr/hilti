// $id$
/// 
/// \addtogroup threading
/// @{
///
/// Functions for manipulating threads.
/// 
/// Note that if not otherwise noted, all the ``hlt_mgr_thread_*`` function
/// is thread-safe. They must only be called from the main control thread.

#ifndef HLT_THREADING_H
#define HLT_THREADING_H

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#include "config.h"

typedef struct hlt_worker_thread hlt_worker_thread;

struct hlt_exception;
struct hlt_continuation;

/// Type for representing the ID of a virtual thread.
typedef int64_t hlt_vthread_id;
#define HLT_VID_MAIN -1

/// The enumeration lists the possible states for a thread manager.
///
/// Note: One could imagine another option such as STOP_WHEN_READY in which
/// new jobs could still be scheduled, but the worker thread would terminate
/// once its job queue becomes empty. This is not currently implemented, but
/// might be useful or even preferable depending on the application.
typedef enum {
    /// RUN means that the worker threads should keep running, even if they run
    /// out of jobs.
    HLT_THREAD_MGR_RUN,

    /// STOP means that the worker threads should disallow any new job
    /// scheduling, and terminate once their job queue is empty.
    HLT_THREAD_MGR_STOP,

    /// KILL means that the worker threads should immediately terminate,
    /// regardless of the contents of their jobs queues.
    HLT_THREAD_MGR_KILL,

    /// DEAD means that the worker threads are dead.
    HLT_THREAD_MGR_DEAD,
        
} hlt_thread_mgr_state;

typedef struct hlt_thread_mgr hlt_thread_mgr;

/// Returns whether the HILTI runtime environment is configured for running
/// multiple threads.
/// 
/// Returns: True if configured for threaded execution.
extern int8_t hlt_is_multi_threaded();

/// Starts the threading system. This must be called after ~~hilti_init but
/// before any other HILTI function is run. 
/// 
/// If the run-time system is not configured for threading, the function is a
/// no-op. 
/// 
/// Returns: The thread manager created for the threading. 
/// 
/// Note: This sets the global ~~__hlt_global_thread_mgr.
extern void hlt_threading_start();

/// Stop the threading system. After this has been called, no further HILTI
/// function may be called. 
/// 
/// When this function returns, all threads will have been shutdown. They
/// will be shutdown gracefully if no unhandled exceptions have be raised,
/// either by any of the workers or passed in via ``excp``. Otherwise, they
/// will be killed immediately.
/// 
/// If no excpeption is passed into the function but one of the threads has
/// raised an unhandled execption, the function returns an
/// UnhandledThreadException.
/// 
/// If the run-time system is not configured for threading, the function is a
/// no-op.
/// 
/// mgr: The mangager returned by ~~hilti_threading_start.
/// 
/// excpt: The raised UnhandledThreadException, if appropiate. 
/// 
/// Note: This destroys the manager stored in ~~__hlt_global_thread_mgr.
extern void hlt_threading_stop(struct hlt_exception** excpt);

/// Creates a new thread manager. A thread manager coordinates a set of
/// worker threads and encapsulates all the state that they share. The new
/// manager will be initialized to state ~~RUN.
/// 
/// This function must only be called if the runtime system has been
/// configured to run with at least one worker thread (see ~~hlt_config_get).
/// 
/// Returns: The new thread manager.
extern hlt_thread_mgr* hlt_thread_mgr_new();

/// Deletes a thread manager. Deleting a manager terminates all still running
/// worker threads and releases all resources associated with them. The
/// manager to be deleted must be in state ~~DEAD.
/// 
/// mgr: The manager to delete.
extern void hlt_thread_mgr_destroy(hlt_thread_mgr* mgr);

/// Transitions a thread manager into a new state.
/// 
/// Currently, the only transitions allowed are from ~~RUN to ~~STOP, and
/// from ~~RUN to ~~KILL. In both cases, the manager will then automatically
/// transition to ~~KILL before the function returns.
/// 
/// mgr: The manager to transition to a new state. 
/// 
/// new_state: The new state.
/// 
/// Note: One could imagine adding the ability to transition back to RUN from
/// the DEAD state in the future, so that a thread context could be reused.
extern void hlt_thread_mgr_set_state(hlt_thread_mgr* mgr, const hlt_thread_mgr_state new_state);

/// Returns a thread manager's current state.
/// 
/// Returns: The current state.
extern hlt_thread_mgr_state hlt_thread_mgr_get_state(const hlt_thread_mgr* mgr);

/// Schedules a job to a virtual thread.
/// 
/// This function is safe to call from all threads.
/// 
/// mgr: The thread manager to use.
/// 
/// vid: The ID of the virtual target thread.
/// 
/// cont: The continuation representing a bound function.
/// 
/// ctx: The caller's execution context.
/// 
/// excpt: &
extern void __hlt_thread_mgr_schedule(hlt_thread_mgr* mgr, hlt_vthread_id vid, struct hlt_continuation* func, struct hlt_execution_context* ctx, struct hlt_exception** excpt);

/// Checks whether any worker thread has raised an uncaught exception. In
/// that case, all worker threads will have been terminated, and this
/// function willl raise an hlt_exception_uncaught_thread_exception.
/// 
/// mgr: The thread manager to use.
/// 
/// excpt: &
extern void hlt_thread_mgr_check_exceptions(hlt_thread_mgr* mgr, struct hlt_exception** excpt);

/// Returns a string identifying the currently running native thread. 
/// 
/// Calling this function is potentially expensive and should be restricted
/// for debugging purposes.
/// 
/// mgr: The thread manager to use. If NULL, the function will return a
/// string indicating that threading is not available.
/// 
/// Returns: A string with a readable identification of the current native thread.
extern const char* hlt_thread_mgr_current_native_thread(hlt_thread_mgr* mgr);

/// @}

#endif
