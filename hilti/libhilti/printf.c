/* $Id$
 * 
 * printf() function for libhilti.
 * 
 */

#include <stdio.h>

#include "hilti.h"

void hilti_print(int32_t n)
{
    printf("%d\n", n);
}
