///
/// libhilti's global state.
///

#include "types.h"

/// Returns the global execution context used by the main thread.
///
/// Returns: The context.
extern hlt_execution_context* hlt_global_execution_context();

/// Returns the thread manager if threading has been initialized, or null
/// otherwise.
///
/// Returns: The thread manager.
extern hlt_thread_mgr* hlt_global_thread_mgr();

/// Initializes all global state. The function is called from hlt_init().
extern void __hlt_global_state_init();

/// Cleans up all global state. The function is called from hlt_done();
extern void __hlt_global_state__done();



