
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "memory_.h"
#include "globals.h"

// Store a configuration set before hlt_init() is called.
static hlt_config* pre_init_config = 0;

static hlt_config* _default_config()
{
    hlt_config* cfg = malloc(sizeof(hlt_config));

    if ( ! cfg ) {
        fputs("cannot allocate memory for libhilti configuration", stderr);
        exit(1);
    }

    const char* dbg = getenv("HILTI_DEBUG");
    if ( ! dbg )
        dbg = "";

    // Set defaults.
    cfg->num_workers = 2;
    cfg->time_idle = 0.1;
    cfg->time_terminate = 1.0;
    cfg->stack_size = 268435456;
    cfg->debug_out = "hlt-debug.log";
    cfg->debug_streams = dbg;
    cfg->profiling = 0;
    cfg->vid_schedule_min = 1;
    cfg->vid_schedule_max = 101;
    cfg->core_affinity = "DEFAULT";

    return cfg;
}

const hlt_config* hlt_config_get()
{
    if ( __hlt_globals() )
        return __hlt_globals()->config;

    if ( pre_init_config )
        return pre_init_config;

    pre_init_config = _default_config();
    return pre_init_config;
}

void hlt_config_set(const hlt_config* new_config)
{
    if ( __hlt_globals() ) {
        *__hlt_globals()->config = *new_config;

        // We copy this over so that it stays valid even when the global
        // context goes away.
        free(__hlt_globals()->debug_streams);
        __hlt_globals()->debug_streams = strdup(new_config->debug_streams);
        return;
    }

    hlt_config_get(); // Make sure pre_init_config is initialized.
    *pre_init_config = *new_config;
    return;
}

void __hlt_config_init()
{
    if ( pre_init_config ) {
        __hlt_globals()->config = pre_init_config;
        pre_init_config = 0;
    }

    else
        __hlt_globals()->config = _default_config();

    __hlt_globals()->debug_streams = strdup(__hlt_globals()->config->debug_streams);
}

void __hlt_config_done()
{
    // Handled outside of the normal memory management.
    free(__hlt_globals()->config);
}
