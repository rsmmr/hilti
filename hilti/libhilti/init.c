/* $Id$
 * 
 * Initialization code that needs to run once at program start.
 * 
 */

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

void hilti_init()
{
    if ( ! setlocale(LC_CTYPE, "") ) {
        fputs("libhilti: cannot set locale", stderr);
        exit(1);
    }
}
