
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "memory_.h"
#include "globals.h"

const hlt_config* hlt_config_get()
{
    if ( ! __hlt_globals()->config ) {
        // Handled outside of the normal memory management.
        __hlt_globals()->config = malloc(sizeof(hlt_config));

        if ( ! __hlt_globals()->config ) {
            fputs("cannot allocate memory for libhilti configuration", stderr);
            exit(1);
        }

        // Set defaults.
        __hlt_globals()->config->num_workers = 2;
        __hlt_globals()->config->time_idle = 0.1;
        __hlt_globals()->config->time_terminate = 1.0;
        __hlt_globals()->config->stack_size = 268435456;
        __hlt_globals()->config->debug_out = "hlt-debug.log";
        __hlt_globals()->config->debug_streams = getenv("HILTI_DEBUG");
        __hlt_globals()->config->profiling = 0;
        __hlt_globals()->config->vid_schedule_min = 1;
        __hlt_globals()->config->vid_schedule_max = 101;
        __hlt_globals()->config->core_affinity = "DEFAULT";
    }

    return __hlt_globals()->config;
}

void hlt_config_set(const hlt_config* new_config)
{
    hlt_config_get(); // Make sure it's initialized.
    *__hlt_globals()->config = *new_config;

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
    free(__hlt_globals()->config);
}
