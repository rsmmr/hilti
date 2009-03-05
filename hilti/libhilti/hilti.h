/* $Id$
 * 
 * Public libhilti interface with functions that can be called directly
 * either from the host application or from a user's HILTI program.
 * 
 */

#ifndef HILTI_H
#define HILTI_H

#include <stdint.h>

typedef struct __hlt_type_info __hlt_type_info;
typedef const char* __hlt_exception;

// Initialize the HILTI run-time library. 
extern void hilti_init();

// Entry point into HILTI processing. 
extern void hilti_run();

// Library functions.
void hilti_print(const __hlt_type_info* type, void* obj, int8_t newline, __hlt_exception* excpt);

typedef struct __hlt_string __hlt_string;
extern void hilti_print_str(const __hlt_string* str);

#endif    
