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
    
}
