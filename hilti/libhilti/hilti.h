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
typedef struct __hlt_string __hlt_string;

typedef struct __hlt_channel __hlt_channel;

    // %doc-hlt_exception-start
typedef const char* __hlt_exception;
    // %doc-hlt_exception-end

// Initialize the HILTI run-time library. 
extern void hilti_init();

// Library functions.
extern void hilti_print(const __hlt_type_info* type, void* obj, int8_t newline, __hlt_exception* excpt);
extern const __hlt_string* hilti_fmt(const __hlt_string* fmt, const __hlt_type_info* type, const char* tuple, __hlt_exception* excpt);

#endif    
