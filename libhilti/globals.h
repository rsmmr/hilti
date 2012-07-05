///
/// libhilti's global state.
///

#include <pthread.h>

#include "hook.h"
#include "types.h"

/// Returns the global execution context used by the main thread.
///
/// Returns: The context. It will *not* have it's cctor applied.
extern hlt_execution_context* hlt_global_execution_context();

/// Returns the thread manager if threading has been initialized, or null
/// otherwise.
///
/// Returns: The thread manager. It will *not* have it's cctor applied.
extern hlt_thread_mgr* hlt_global_thread_mgr();

/// Sets the global thread manager. For internal use only, must only be
/// called by the threading code.
extern void __hlt_global_set_thread_mgr(hlt_thread_mgr* mgr);

/// Initializes all global state. The function is called from hlt_init().
extern void __hlt_global_state_init();

/// Cleans up all global state. The function is called from hlt_done();
extern void __hlt_global_state__done();

/// The current hook state. This must only modified by
/// ~~__hlt_hook_group_enable, which protects accesses via the lock.
extern hlt_hook_state* __hlt_global_hook_state;
extern pthread_mutex_t __hlt_global_hook_state_lock;

/// Flag to signal worker threads to terminate. This must only be accessed by
/// the thread mgr. Use __hlt_thread_mgr_terminating() to query.
extern int8_t __hlt_global_thread_mgr_terminate;

