/* $Id$
 * 
 * Internal exception handling functions. 
 * 
 * Note: This implementation of exceptions is preliminary and will very
 * likely change in the future.
 * 
 */

#include <unistd.h>

#include "hilti.h"
#include "string.h"

/* Predefined exceptions. */
const char* __hilti_exception_unspecified = "Unspecified";
const char* __hilti_exception_division_by_zero = "DivisionByZero";

void __hilti_exception_print_uncaught(void* exception) {
    // This is quite a hack currently ...
    const char* name = (const char*)exception; 

    write(2, "hilti: uncaught exception, ", 28);
    write(2, name, strlen(name));
    write(2, "\n", 1);
}

