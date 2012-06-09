
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "memory_.h"

// FIXME: Maybe it would be better to have the user create a config object and use it,
// instead of having a singleton one.
static hlt_config* _current_config = 0;

const hlt_config* hlt_config_get()
{
    if ( ! _current_config ) {
        // Handled outside of the normal memory management.
        _current_config = malloc(sizeof(hlt_config));

        if ( ! _current_config ) {
            fputs("cannot allocate memory for libhilti configuration", stderr);
            exit(1);
        }

        // Set defaults.
        _current_config->num_workers = 2;
        _current_config->time_idle = 0.1;
        _current_config->time_terminate = 1.0;
        _current_config->stack_size = 268435456;
        _current_config->debug_out = "hlt-debug.log";
        _current_config->debug_streams = getenv("HILTI_DEBUG");
        _current_config->profiling = 0;
        _current_config->vid_schedule_min = 1;
        _current_config->vid_schedule_max = 101;
        _current_config->core_affinity = "DEFAULT";
    }

    return _current_config;
}

void hlt_config_set(const hlt_config* new_config)
{
    hlt_config_get(); // Make sure it's initialized.
    *_current_config = *new_config;

    // TODO: Need to deal with ref-cnt list for debug streams here.
}

void __hlt_config_init()
{
    // Make sure we've initialized our config.
    hlt_config_get();
}

void __hlt_config_done()
{
    // Handled outside of the normal memory management.
    free(_current_config);
}
