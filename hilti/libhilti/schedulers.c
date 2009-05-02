/* $Id$
 *
 * Schedulers for HILTI's threading support. The schedulers are implemented using the internal API
 * defined in thread_context.h.
 * 
 */

#include <stdio.h>
#include <stdint.h>

#include "hilti_intern.h"

// Maps a vthread onto an actual worker thread. An FNV-1a hash is used to distribute the
// vthreads as evenly as possible between the worker threads. This algorithm should be
// fairly fast, but if profiling reveals __hlt_thread_from_vthread to be a bottleneck, it
// can always be replaced with a simple mod function. The desire to perform this mapping
// quickly should be balanced with the desire to distribute the vthreads equitably, however,
// as an uneven distribution could result in serious performance issues for some applications.
static uint32_t __hlt_thread_from_vthread(const uint32_t vthread, const uint32_t num_threads)
{
    // Some constants for the 32-bit FNV-1 hash algorithm.
    const uint32_t FNV_32_OFFSET_BASIS = 2166136261;
    const uint32_t FNV_32_PRIME = 16777619;
    const uint32_t FNV_32_MASK = 255;

    // Initialize.
    uint32_t hash = FNV_32_OFFSET_BASIS;

    // Perform a 32-bit FNV-1 hash.
    for (unsigned i = 0 ; i < 4 ; i++)
    {
        hash = hash ^ ((vthread >> (8 * i)) & FNV_32_MASK);
        hash = hash * FNV_32_PRIME;
    }

    // Map the hash onto the number of worker threads we actually have.
    // Note that using mod here introduces a slight amount of bias, but the non-biased
    // algorithm makes the time taken by this function unpredictable. Further investigation
    // may be warranted, but for now I've used mod, which should still exhibit low bias
    // and completes in a fixed amount of time regardless of the input.
    hash = hash % num_threads;

    return hash;
}

void __hlt_global_schedule_job(uint32_t vthread, __hlt_hilti_function function, __hlt_hilti_frame frame)
{
    // Get information about the current thread context.
    __hlt_thread_context* context = __hlt_get_current_thread_context();
    uint32_t num_threads = __hlt_get_thread_count(context);

    // Map the vthread to a worker thread.
    uint32_t thread_id = __hlt_thread_from_vthread(vthread, num_threads);

    // Schedule the job.
    __hlt_schedule_job(context, thread_id, function, frame);
}

void __hlt_local_schedule_job(__hlt_hilti_function function, __hlt_hilti_frame frame)
{
    // Get information about the current thread context.
    __hlt_thread_context* context = __hlt_get_current_thread_context();
    uint32_t thread_id = __hlt_get_current_thread_id();

    // Schedule the job to the current thread.
    __hlt_schedule_job(context, thread_id, function, frame);
}
