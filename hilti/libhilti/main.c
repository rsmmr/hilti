/* $Id$
 * 
 * Implementation of main() which directly calls first hilti_init() and then
 * hilti_run().
 * 
 */

#include "hilti.h"

int main(int argc, char** argv)
{
    hilti_init();
    hilti_run();
    return 0;
}
