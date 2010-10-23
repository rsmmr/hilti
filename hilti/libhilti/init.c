/* $Id$
 * 
 * Initialization code that needs to run once at program start.
 * 
 */

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "hilti.h"
#include "context.h"

typedef void (*init_func)();

static init_func* _registered_funcs = 0;
static int _num_registered_funcs = 0;

void hlt_init()
{
    // Initialize locale.
    if ( ! setlocale(LC_CTYPE, "") ) {
        fputs("libhilti: cannot set locale", stderr);
        exit(1);
    }

    // Initialize configuration to defaults.
    __hlt_config_init();
    
    // Initialize the garbage collector.
    __hlt_init_gc();

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

    for ( int i = 0; i < _num_registered_funcs; i++ )
        (*(_registered_funcs[i]))();
        

    // This is easy to forget ...
    __hlt_debug_printf_internal("hilti-trace", "Reminder: hilti-trace requires compiling with debugging level > 1.");
    __hlt_debug_printf_internal("hilti-flow",  "Reminder: hilti-flow requires compiling with debugging level > 1.");
}

void hlt_register_init_function(init_func func, hlt_exception* expt)
{
    ++_num_registered_funcs;
    _registered_funcs = hlt_gc_realloc_atomic(_registered_funcs, _num_registered_funcs * sizeof(init_func));
    _registered_funcs[_num_registered_funcs - 1] = func;
}
