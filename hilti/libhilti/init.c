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

void hilti_init()
{
    // Initialize locale.
    if ( ! setlocale(LC_CTYPE, "") ) {
        fputs("libhilti: cannot set locale", stderr);
        exit(1);
    }

    // Initialize configuration to defaults.
    __hlt_config_init();

    if ( ! hlt_is_multi_threaded() )
        __hlt_execution_context_init(&__hlt_global_execution_context);

    // Initialize debug streams from environment. 
    const char* dbg = getenv("HILTI_DEBUG");
    if ( dbg ) {
        hlt_exception* excpt = 0;
        hilti_config cfg = *hilti_config_get();
        cfg.debug_streams = hlt_debug_parse_streams(dbg, &excpt);
        if ( excpt ) {
            hlt_exception_print(excpt);
            fprintf(stderr, "cannot parse HILTI_DEBUG environment variable");
            exit(1);
        }
        
        hilti_config_set(&cfg);
    }
}
