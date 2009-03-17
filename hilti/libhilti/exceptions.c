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

__hlt_exception __hlt_exception_division_by_zero = "DivisionByZero";
__hlt_exception __hlt_exception_value_error = "ValueError";
__hlt_exception __hlt_exception_out_of_memory = "OutOfMemory";
__hlt_exception __hlt_exception_wrong_arguments = "WrongArguments";
__hlt_exception __hlt_exception_undefined_value = "UndefinedValue";
__hlt_exception __hlt_exception_unspecified = "Unspecified";

void __hlt_exception_print_uncaught(__hlt_exception exception) {
    // This is quite a hack currently ...
    const char* name = (const char*)exception; 

    write(2, "hilti: uncaught exception, ", 28);
    write(2, name, strlen(name));
    write(2, "\n", 1);
}

