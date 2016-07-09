
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "globals.h"
#include "memory_.h"

hlt_config* __hlt_default_config()
{
    hlt_config* cfg = malloc(sizeof(hlt_config));

    if ( ! cfg ) {
        fputs("cannot allocate memory for libhilti configuration", stderr);
        exit(1);
    }

    const char* dbg = getenv("HILTI_DEBUG");
    if ( ! dbg )
        dbg = "";

    const char* profile = getenv("HILTI_PROFILE");

    // Set defaults.
    cfg->num_workers = 2;
    cfg->time_idle = 0.1;
    cfg->time_terminate = 1.0;
    cfg->thread_stack_size = 2684354560;       // This is generous.
    cfg->fiber_stack_size = 100 * 1024 * 1024; // This is generous.
    cfg->fiber_max_pool_size = 1000;
    cfg->debug_out = "hlt-debug.log";
    cfg->debug_streams = dbg;
    cfg->profiling = (profile && *profile);
    cfg->vid_schedule_min = 1;
    cfg->vid_schedule_max = 101;
    cfg->core_affinity = "DEFAULT";

    return cfg;
}

extern __hlt_global_state* __hlt_globals_object_no_init();

const hlt_config* hlt_config_get()
{
    return __hlt_globals_object_no_init()->config;
}

void hlt_config_set(const hlt_config* new_config)
{
    __hlt_global_state* globals = __hlt_globals_object_no_init();

    if ( globals->initialized ) {
        fputs("libhilti: hlt_config_set() called after runtime initializion", stderr);
        abort();
    }

    *globals->config = *new_config;

    // We copy this over so that it stays valid even when the global
    // context goes away.
    free(globals->debug_streams);
    globals->debug_streams = strdup(new_config->debug_streams);
}

void hlt_config_dump(FILE* f)
{
    hlt_config* cfg = __hlt_globals_object_no_init()->config;

    fprintf(f, "=== libhilti global configuration (at %p)\n", cfg);
    fprintf(f, "num_workers:         %u\n", cfg->num_workers);
    fprintf(f, "time_idle:           %.2f\n", cfg->time_idle);
    fprintf(f, "time_terminate:      %.2f\n", cfg->time_terminate);
    fprintf(f, "thread_stack_size:   %zu\n", cfg->thread_stack_size);
    fprintf(f, "fiber_stack_size:    %zu\n", cfg->fiber_stack_size);
    fprintf(f, "fiber_max_pool_size: %zu\n", cfg->fiber_max_pool_size);
    fprintf(f, "debug_out:           %s\n", cfg->debug_out);
    fprintf(f, "debug_streams:       %s\n", cfg->debug_streams);
    fprintf(f, "profiling:           %s\n", (cfg->profiling ? "yes" : "no"));
    fprintf(f, "vid_schedule_min:    %" PRId64 "\n", cfg->vid_schedule_min);
    fprintf(f, "vid_schedule_max:    %" PRId64 " \n", cfg->vid_schedule_max);
    fprintf(f, "core_affinity:       %s\n", cfg->core_affinity);
}
