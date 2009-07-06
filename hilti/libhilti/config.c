/* $Id$
 *
 * Configuration mechanism for the HILTI framework and runtime.
 * 
 */

#include "hilti.h"

// FIXME: Maybe it would be better to have the user create a config object and use it,
// instead of having a singleton one.

static hilti_config current_config;

void __hlt_config_init()
{
   current_config.num_threads = 2;
   current_config.sleep_ns = 10000000;
   current_config.watchdog_s = 1;
   current_config.stack_size = 268435456; 
   current_config.debug_out = stderr;
   current_config.debug_streams = 0;
}

const hilti_config* hilti_config_get()
{
    return &current_config;
}

void hilti_config_set(const hilti_config* new_config)
{
    current_config = *new_config;
}
