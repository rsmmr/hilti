///
/// libhilti's global state.
///

#ifndef LIBHILTI_GLOBALS_H
#define LIBHILTI_GLOBALS_H

#include <pthread.h>
#include <stdio.h>

#include "hook.h"
#include "types.h"

// A struct holding all of libhilti's internal global variables.
//
// Note: Global variables should be limited to a mininum, and must be ensured
// that they are used in a thread-safe fashion. All globals *must be* listed
// in this struct, no exception (other than the global pointer to this struct
// :)

struct __hlt_global_state {
    hlt_execution_context* context;    // The global execution context.

    // These are managed by the corresponding subsystems. Access shouldn't be
    // happening from elsehwere. They must be initialized by their
    // corresponding *_init() functions (though they are guarenteed to be set
    // to zero initially).

    // config.c
    hlt_config* config;

    // config.c and debug.c
    // We maintain this here to keep it around even after the global
    // context has gone away.
    char* debug_streams;

    // cmdqueue.c
    hlt_thread_queue* cmd_queue;
    pthread_t queue_manager;

    // threading.c
    hlt_thread_mgr*        thread_mgr; // The global thread manager.
    int8_t multi_threaded;
    int8_t thread_mgr_terminate;

    // hook.c
    hlt_hook_state* hook_state;
    pthread_mutex_t hook_state_lock; // Lock to protect access to hook_state.

    // file.c
    __hlt_file_info* files;
    pthread_mutex_t files_lock; // Lock to protect access to files.

    // profile.c
    int8_t profiling_enabled;
    int8_t papi_available;
    int papi_set;

    // The following are for debugging only. However, we can't compile them
    // out in the non-debugging version because a host application might link
    // to a different runtime version that compiled code, but both may still
    // access the same global state object. (Actually having these at the end
    // could work, but don't want to rely on that.)
    uint64_t job_counter;
    FILE* debug_out;
    _Atomic(uint_fast64_t) debug_counter;
    _Atomic(uint_fast64_t) num_allocs;
    _Atomic(uint_fast64_t) num_deallocs;
    _Atomic(uint_fast64_t) num_refs;
    _Atomic(uint_fast64_t) num_unrefs;
};

// A type holding all of libhilti's global state.
typedef struct __hlt_global_state __hlt_global_state;

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

/// Cleans up all global state. The function is called from hlt_done(). Note
/// that when multiple instance of libhilti share a set of global state (via
/// \a __hlt_global_state_set), they all must call this function before it's
/// gets released. Note however that this function is not thread-safe.
///
/// \todo: Should it be thread-safe?
extern void __hlt_global_state_done();

/// Returns a pointer to the set of global state. This is primarily for use
/// with \a __hlt_global_state_set().
///
/// \todo: Should this function be thread-safe?
extern __hlt_global_state*  __hlt_globals();

/// Instructs libhilti to use a different set of global state. This can be
/// used when a process has loaded libhilti twice (e.g., itself and via JIT),
/// to sync the two instances. This can be used instead or after
/// __hlt_global_state_init().
///
/// \todo: Should this function be thread-safe?
extern void __hlt_globals_set(__hlt_global_state* state);

#endif
