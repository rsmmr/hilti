// $Id$
//
// Support functions for HILTI's integer data type.

#ifndef HILTI_CONFIG_H
#define HILTI_CONFIG_H

#include <stdio.h>

struct __hlt_list;

/// Configuration parameters for the HILTI run-time system.
typedef struct
{
    /// Number of worker threads to spawn.
    unsigned num_workers;

    /// Seconds to sleep when worker thread is idle.
    double time_idle;

    /// Secs o wait for threads to termintate after asking them to stop.
    double time_terminate;

    /// Stack size for worker threads.
    size_t stack_size;

    /// Where debug output is to be sent. Default is stderr.
    FILE* debug_out;

    /// List of enabled debug output streams. This is of type
    /// ``list<string>``. Per default, the list is empty.
    struct __hlt_list* debug_streams;
} hlt_config;

extern const hlt_config* hlt_config_get();

// This must not been called after hlt_init.
extern void hlt_config_set(const hlt_config* new_config);

extern void __hlt_config_init();

#endif




