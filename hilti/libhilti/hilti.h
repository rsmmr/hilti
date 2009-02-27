/* $Id$
 * 
 * Public libhilti interface with functions that can be called directly
 * either from the host application or from a user's HILTI program.
 * 
 */

#ifndef HILTI_H
#define HILTI_H

#include <stdint.h>

// Intialize the HILTI run-time librart. 
extern void hilti_init();

// Entry point into HILTI processing. 
extern void hilti_run();

// Library functions.
extern void hilti_print(int32_t n);

struct __hlt_string;
extern void hilti_print_str(const struct __hlt_string* str);

#endif    
