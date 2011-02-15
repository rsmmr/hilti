/* $Id$
 *
 * Initialization code that needs to run once at program start.
 *
 */

#include "hilti.h"
#include "context.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int8_t prio;
    void (*func)();
} init_func;

static init_func* _registered_funcs = 0;
static int _num_registered_funcs = 0;

void hlt_init()
{
    // Initialize the garbage collector.
    __hlt_gc_init();

    // Initialize locale.
    if ( ! setlocale(LC_CTYPE, "") ) {
        fputs("libhilti: cannot set locale", stderr);
        exit(1);
    }

    // Initialize configuration to defaults.
    __hlt_config_init();

    // Initialize profiling.
    __hlt_profiler_init();

    // Initialize debug streams from environment.
    const char* dbg = getenv("HILTI_DEBUG");
    if ( dbg ) {
        hlt_exception* excpt = 0;
        hlt_config cfg = *hlt_config_get();
        cfg.debug_streams = hlt_debug_parse_streams(dbg, &excpt, __hlt_global_execution_context);
        if ( excpt ) {
            hlt_exception_print(excpt, __hlt_global_execution_context);
            fprintf(stderr, "cannot parse HILTI_DEBUG environment variable");
            exit(1);
        }

        hlt_config_set(&cfg);
    }

    // Two rounds for the two priortity levels.
    int i;
    
    for ( i = 0; i < _num_registered_funcs; i++ )
        if ( _registered_funcs[i].prio > 0 )
            (*(_registered_funcs[i].func))();

    for ( i = 0; i < _num_registered_funcs; i++ )
        if ( _registered_funcs[i].prio == 0 )
            (*(_registered_funcs[i].func))();

    // This is easy to forget ...
    __hlt_debug_printf_internal("hilti-trace", "Reminder: hilti-trace requires compiling with debugging level > 1.");
    __hlt_debug_printf_internal("hilti-flow",  "Reminder: hilti-flow requires compiling with debugging level > 1.");
}

void hlt_register_init_function(void (*func)(), int8_t prio, hlt_exception* excpt, hlt_execution_context* ctx)
{
    ++_num_registered_funcs;
    _registered_funcs = hlt_gc_realloc_atomic(_registered_funcs, _num_registered_funcs * sizeof(init_func));

    init_func ifunc = { prio, func };
    _registered_funcs[_num_registered_funcs - 1] = ifunc;
}
