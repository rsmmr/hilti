/* $Id$
 * 
 * Implementation of main() which directly calls first hilti_init() and then
 * hilti_multithreaded_run().
 * 
 */

#include "hilti_intern.h"

int main(int argc, char** argv)
{
    __hlt_exception excpt = 0;
    
    hilti_init();

    // Initialize debug streams from environment. 
    const char* dbg = getenv("HILTI_DEBUG");
    if ( dbg ) {
        hilti_config cfg = *hilti_config_get();
        cfg.debug_streams = __hlt_debug_parse_streams(dbg, &excpt);
        if ( excpt ) {
            __hlt_exception_print(excpt);
            fprintf(stderr, "cannot parse HILTI_DEBUG environment variable");
            return 1;
        }
        hilti_config_set(&cfg);
    }
    
    // Using this function is OK even in the single-threaded case, as
    // hilti_multithreaded_run won't create a threading environment
    // if the config only requests one thread. You can change the defaults
    // in config.c or use hilti_get_config() and hilti_set_config() to
    // change the configuration as needed.
    hilti_multithreaded_run(&excpt);

    if ( excpt )
        __hlt_exception_print_uncaught(excpt);
    
    return 0;
}
