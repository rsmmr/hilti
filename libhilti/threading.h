
#ifndef HLT_THREADING_H
#define HLT_THREADING_H

#include "types.h"

/// Returns whether the HILTI runtime environment is configured for running
/// multiple threads.
///
/// Returns: True if configured for threaded execution.
extern int8_t hlt_is_multi_threaded();

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

/// Type for representing the ID of a virtual thread.
#define HLT_VID_MAIN -1
#define HLT_VID_QUEUE -2

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

// FIXME: Dumy for now.
typedef struct __hlt_worker_thread {
    int id;                       // ID of this worker thread in the range 1..*num_workers*.
} hlt_worker_thread;

// FIXME: Dummy for now.
// A thread manager encapsulates the global state that all threads share.
struct __hlt_thread_mgr {
    hlt_thread_mgr_state state;  // The manager's current state.
};

#endif
