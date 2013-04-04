//
// Infrastructure to configure libhilti runtime behaviour.
//

#ifndef LIBHILTI_CONFIG_H
#define LIBHILTI_CONFIG_H

#include <stdio.h>

#include "types.h"

/// Configuration parameters for the HILTI runtime system..
struct __hlt_config
{
    /// Number of worker threads to spawn.
    unsigned num_workers;

    /// Seconds to sleep when worker thread is idle.
    double time_idle;

    /// Secs o wait for threads to termintate after asking them to stop.
    double time_terminate;

    /// Stack size for worker threads.
    size_t thread_stack_size;

    /// Stack size for fibers.
    size_t fiber_stack_size;

    /// File where debug output is to be sent. Default is stderr.
    const char* debug_out;

    /// Colon-separated list of debug stream.
    const char* debug_streams;

    /// 1 if profiling is enabled, 0 otherwise. Default is off.
    int8_t profiling;

    /// The smallest virtual thread number to use when hashing a thread
    /// context into the set of virtual threads. Default is 1.
    hlt_vthread_id vid_schedule_min;

    /// The largest virtual thread number to use when hashing a thread
    /// context into the set of virtual threads. Default is 100.
    hlt_vthread_id vid_schedule_max;

    /// A string specifying thread-to-core affinity with entries of the form
    /// "thread-name:core-number", separated by commas. No whitespace allowed
    /// anywhere. Set to NULL or empty string to disable any pinning. Default
    /// is the magic string "DEFAULT" which let's HILTI determine a pinning
    /// itself.
    const char* core_affinity;

};

/// Returns the current configuration. The returned value cannot be directly
/// modified. To change a setting, copy it by value and modify as desired.
/// The pass the new instance to hlt_config_set().
///
/// Returns: The current configuration.
extern const hlt_config* hlt_config_get();

/// Sets a new configuration for the libhilti. Normally, you should get the
/// current information from hlt_config_get() and only modify what you want
/// to change.
///
/// This function must only be called before hlt_init().
///
/// new_config: The new configuration. The passed in object will value-copied
/// internally so the instance can be deleted afterwards.
extern void hlt_config_set(const hlt_config* new_config);

/// Initializes libhilti's configuration subsystem. The function is called
/// from hlt_init().
extern void __hlt_config_init();

/// Terminates libhilti's debugging subsystem. The function is called from
/// hlt_done().
extern void __hlt_config_done();

#endif




