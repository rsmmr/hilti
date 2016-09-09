///
/// libhilti's global state.
///

#ifndef LIBHILTI_GLOBALS_H
#define LIBHILTI_GLOBALS_H

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C++" {
#include <atomic>
typedef std::atomic_uint_fast64_t atomic_uint_fast64_t;
}
#else
#include <stdatomic.h>
#endif

#include "hook.h"
#include "types.h"

// A struct holding all of libhilti's internal global variables.
//
// Note: Global variables should be limited to a mininum, and must be ensured
// that they are used in a thread-safe fashion. All globals *must be* listed
// in this struct, no exception.
struct __hlt_global_state {
    // Flag indicating __hlt_global_state_init() has completed.
    int initialized;

    // Flag indicating __hlt_global_state_done() has completed.
    int finished;

    // Size of the space we need for global HILTI variables.
    uint64_t globals_size;

    // The global execution context.
    hlt_execution_context* context;

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
    hlt_execution_context* cmd_context;
    pthread_t queue_manager;

    // threading.c
    hlt_thread_mgr* thread_mgr; // The global thread manager.
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

    // timer.c
    atomic_uint_fast64_t global_time;

    // fiber.c
    __hlt_fiber_pool* synced_fiber_pool;    // Global fiber pool.
    pthread_mutex_t synced_fiber_pool_lock; // Lock to protect access to pool.

    // The following are for debugging only. However, we can't compile them
    // out in the non-debugging version because a host application might link
    // to a different runtime version that compiled code, but both may still
    // access the same global state object. (Actually having these at the end
    // could work, but don't want to rely on that.)
    uint64_t job_counter;
    FILE* debug_out;
    atomic_uint_fast64_t debug_counter;
    atomic_uint_fast64_t num_allocs;
    atomic_uint_fast64_t num_deallocs;
    atomic_uint_fast64_t num_refs;
    atomic_uint_fast64_t num_unrefs;
    atomic_uint_fast64_t num_stacks;
    atomic_uint_fast64_t size_stacks;
    atomic_uint_fast64_t num_nullbuffer;
    atomic_uint_fast64_t max_nullbuffer;
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
///
/// The function is safe against multiple executions; all calls after the
/// first will be ignored. Returns true if it was the first execution, and
/// false otherwise.
extern int __hlt_global_state_init();

/// Cleans up all global state. The function is called from hlt_done().
///
/// The function is safe against multiple executions; all calls after the
/// first will be ignored. Returns true if it was the first execution after
/// __hlt_global_state_init(), and false otherwise.
extern int __hlt_global_state_done();

/// Returns a pointer to the set of global state.
extern __hlt_global_state* __hlt_globals();

/// Dumps out a debug representation of the global state.
///
/// f: The file handle to write to.
extern void hlt_global_state_dump(FILE* f);

extern void* __hlt_runtime_state_get();
extern void __hlt_runtime_state_set(void* state);

#endif
