/* $Id$
 * 
 * Internal exception handling functions. 
 * 
 * Note: This implementation of exceptions is preliminary and will very
 * likely change in the future.
 * 
 */

#include <unistd.h>
#include <string.h>

#include "hilti_intern.h"

__hlt_exception_t __hlt_exception_unspecified = "Unspecified";
__hlt_exception_t __hlt_exception_division_by_zero = "DivisionByZero";
__hlt_exception_t __hlt_exception_value_error = "ValueError";
__hlt_exception_t __hlt_exception_out_of_memory = "OutOfMemory";

void __hlt_exception_print_uncaught(__hlt_exception_t exception) {
    // This is quite a hack currently ...
    const char* name = (const char*)exception; 

    write(2, "hilti: uncaught exception, ", 28);
    write(2, name, strlen(name));
    write(2, "\n", 1);
}

void __hlt_exception_raise(__hlt_exception_t exception)
{
    // Todo: Implement somehow ... This is just temporary.
    const char* name = (const char*)exception; 
    
    write(2, "hiltilib raised exception but we can't do that yet: ", 52);
    write(2, name, strlen(name));
    write(2, "\n", 1);
    abort();
}

