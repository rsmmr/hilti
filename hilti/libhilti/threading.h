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
#include "context.h"
#include "tqueue.h"

#define DBG_STREAM       "hilti-threads"
#define DBG_STREAM_STATS "hilti-threads-stats"

struct hlt_exception;
struct hlt_continuation;
struct __hlt_type_info;

/// The enumeration lists the possible states for a thread manager.
///
/// Note: One could imagine another option such as STOP_WHEN_READY in which
/// new jobs could still be scheduled, but the worker thread would terminate
/// once its job queue becomes empty. This is not currently implemented, but
/// might be useful or even preferable depending on the application.
typedef enum {
    /// NEW means a freshly initialized manager whose worker threads haven't
    /// started yet.
    HLT_THREAD_MGR_NEW,

    /// RUN means that the worker threads should keep running, even if they run
    /// out of jobs.
    HLT_THREAD_MGR_RUN,

    /// FINISH menas that worker should terminate once they are all idle.
    HLT_THREAD_MGR_FINISH,

    /// STOP means that the worker threads should disallow any new job
    /// scheduling, and terminate once their job queue is empty.
    HLT_THREAD_MGR_STOP,

    /// KILL means that the worker threads should immediately terminate,
    /// regardless of the contents of their jobs queues.
    HLT_THREAD_MGR_KILL,

    /// DEAD means that the worker threads are dead.
    HLT_THREAD_MGR_DEAD,

} hlt_thread_mgr_state;

typedef struct hlt_thread_mgr_blockable {
    uint64_t num_blocked;	// Number of jobs waiting for this resource.
} hlt_thread_mgr_blockable;

// A job queued for execution.
typedef struct hlt_job {
    hlt_continuation* func; // The bound function to run.
    hlt_vthread_id vid;     // The virtual thread the function is scheduled to.
    void* tcontext;         // The jobs thread context to use when executing.

#ifdef DEBUG
    uint64_t id;            // For debugging, we assign numerical IDs for easier identification.
#endif

    struct hlt_job* prev;   // The prev job in the *blocked* queue if linked in there.
    struct hlt_job* next;   // The next job in the *blocked* queue if linked in there.
} hlt_job;

struct hlt_thread_mgr;

// A struct that encapsulates data related to a single worker thread.
typedef struct hlt_worker_thread {
    // Accesses to these must only be made from the worker thread itself.
    struct hlt_thread_mgr* mgr;   // The manager this thread is part of.
    hlt_execution_context** ctxs; // Execution contexts indexed by virtual thread id.
    hlt_vthread_id max_vid;       // Largest vid allocated space for in ctxs.

    // This can be *read* from different threads without further locking.
    int id;                       // ID of this worker thread in the range 1..*num_workers*.
    const char* name;             // A string identifying the worker.
    int idle;                     // When in state FINISH, the worker will set this when idle.
    pthread_t handle;             // The pthread handle for this thread.

    // Write accesses to the main jobs queue can be made from all worker
    // threads and the main thread, while read accesses come only from the
    // worker thread itself. If another thread needs to schedule a job
    // already blocked, it can write here and it will be moved over to the
    // blocked queue by the worker later.
    hlt_thread_queue* jobs;  // Jobs queued for this worker and ready to run.

    // Write accesses to the blocked queue  may be done only from the writer.
    hlt_job* blocked_head;
    hlt_job* blocked_tail;
    uint64_t num_blocked_jobs;
} hlt_worker_thread;

// A thread manager encapsulates the global state that all threads share.
typedef struct hlt_thread_mgr {
    hlt_thread_mgr_state state;  // The manager's current state.
    int num_workers;             // The number of worker threads.
    int num_excpts;              // The number of worker's that have raised exceptions.
    hlt_worker_thread** workers; // The worker threads.
    pthread_t cmdqueue;          // The pthread handle for the command queue.
    pthread_key_t id;            // A per-thread key storing a string identifying the string.
} hlt_thread_mgr;


/// Type for representing the ID of a virtual thread.
#define HLT_VID_MAIN -1
#define HLT_VID_QUEUE -2

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
/// Note: This deletes the manager stored in ~~__hlt_global_thread_mgr.
extern void hlt_threading_stop(struct hlt_exception** excpt);

/// Returns an estimate of the current scheduler load. The load is normalized
/// into the range from zero (low) to one (high). If on the higher end, it
/// should consider reducing new load if possible.
///
/// excpt: &
extern double hlt_threading_load(struct hlt_exception** excpt);

/// Creates a new thread manager. A thread manager coordinates a set of
/// worker threads and encapsulates all the state that they share. The new
/// manager will be initialized to state ~~NEW.
///
/// This function must only be called if the runtime system has been
/// configured to run with at least one worker thread (see ~~hlt_config_get).
///
/// Returns: The new thread manager.
extern hlt_thread_mgr* hlt_thread_mgr_new();

/// Starts the thread manager's worker thread processing. The manager must be
/// in state ~~NEW and will be switched to ~~RUN.
///
/// This function must only be called if the runtime system has been
/// configured to run with at least one worker thread (see ~~hlt_config_get).
///
/// mgr: The manager to start.
extern void hlt_thread_mgr_start(hlt_thread_mgr*);

/// Deletes a thread manager. Deleting a manager terminates all still running
/// worker threads and releases all resources associated with them. The
/// manager to be deleted must be in state ~~DEAD.
///
/// mgr: The manager to delete.
extern void hlt_thread_mgr_delete(hlt_thread_mgr* mgr);

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
extern void __hlt_thread_mgr_schedule(hlt_thread_mgr* mgr, hlt_vthread_id vid, struct hlt_continuation* func, struct hlt_exception** excpt, struct hlt_execution_context* ctx);

/// Schedules a job to a virtual thread determined by an object's hash.
///
/// This function determined the target virtual thread by hashing *obj* into
/// the range determined by ``config.vid_schedule_max -
/// config.vid_schedule_min``.
/// 
/// This function is safe to call from all threads.
///
/// mgr: The thread manager to use.
///
/// type: The type of the thread context to be hashed (i.e., *tcontext*).
///
/// tcontext: The thread context for the job, per its module's ``context`` definition.
///
/// cont: The continuation representing a bound function.
///
/// ctx: The caller's execution context.
///
/// excpt: &
extern void __hlt_thread_mgr_schedule_tcontext(hlt_thread_mgr* mgr, const struct __hlt_type_info* type, void* tcontext, struct hlt_continuation* func, struct hlt_exception** excpt, struct hlt_execution_context* ctx);

/// Checks whether any worker thread has raised an uncaught exception. In
/// that case, all worker threads will have been terminated, and this
/// function willl raise an hlt_exception_uncaught_thread_exception.
///
/// mgr: The thread manager to use.
///
/// excpt: &
extern void hlt_thread_mgr_check_exceptions(hlt_thread_mgr* mgr, struct hlt_exception** excpt, struct hlt_execution_context* ctx);

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

/// Initialized a new native thread. Must be called once at the startup of
/// any new native thread *from that thread*. It sets a name identifying the
/// native thread, and also pins it to a core if specified so in the HILTI
/// configuration.
///
/// mgr: The thread manager to use. If NULL, the function will return without doing anything.
///
/// name: A string with a readable identification of the current native thread.
///
/// default_affinity: A core to pin the thread to if the HILTI is configured
/// to use its default affinity scheme (see config.h). Can be -1 to set no
/// affinity by default for this thread.
extern void __hlt_thread_mgr_init_native_thread(hlt_thread_mgr* mgr, const char* name, int default_affinity);

inline static void hlt_thread_mgr_blockable_init(hlt_thread_mgr_blockable* resource)
{
    resource->num_blocked = 0;
}

extern void __hlt_thread_mgr_unblock(hlt_thread_mgr_blockable *resource, hlt_execution_context* ctx);

inline static void hlt_thread_mgr_unblock(hlt_thread_mgr_blockable *resource, hlt_execution_context* ctx)
{
    if ( ! resource->num_blocked )
        return;

    __hlt_thread_mgr_unblock(resource, ctx);
}

/// @}

#endif
