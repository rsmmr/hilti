// $Id$
// 
// Support functions for HILTI's integer data type.

#ifndef HILTI_CONFIG_H
#define HILTI_CONFIG_H

#include "exceptions.h"
#include "list.h"

// Configuration handling structures and functions for the HILTI run-time.
typedef struct
{
    unsigned num_threads;
    long sleep_ns;
    unsigned watchdog_s;
    size_t stack_size;

    // Where debug output is to be sent. Default is stderr.
    FILE* debug_out;
    
    // List of enabled debug output streams. Default is none.
    hlt_list* debug_streams; // list<string> 
} hilti_config;

extern const hilti_config* hilti_config_get();
extern void hilti_config_set(const hilti_config* new_config);

extern void __hlt_config_init();

#endif




