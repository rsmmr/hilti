/* $Id$
 * 
 * Libhilti interface definitions.
 * 
 */

#ifndef HILTI_H
#define HILTI_H

#include <stdint.h>

// Entry point into HILTI processing. 
extern void hilti_run();

// Library functions.
extern void hilti_print(int32_t n);

#endif    
