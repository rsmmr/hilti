/* $Id$
 * 
 * Implementation of main() which directly calls first hilti_init() and then
 * hilti_run().
 * 
 */

#include "hilti_intern.h"

// Entry point into HILTI processing. 
extern void main_run(const __hlt_exception* excpt);

int main(int argc, char** argv)
{
    hilti_init();
    
    __hlt_exception excpt = 0;
    main_run(&excpt);
    
    if ( excpt )
        __hlt_exception_print_uncaught(excpt);
    
    return 0;
}
