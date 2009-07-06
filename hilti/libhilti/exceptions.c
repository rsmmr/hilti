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
#include <stdio.h>

#include "hilti_intern.h"

__hlt_exception __hlt_exception_division_by_zero = "DivisionByZero";
__hlt_exception __hlt_exception_value_error = "ValueError";
__hlt_exception __hlt_exception_out_of_memory = "OutOfMemory";
__hlt_exception __hlt_exception_wrong_arguments = "WrongArguments";
__hlt_exception __hlt_exception_undefined_value = "UndefinedValue";
__hlt_exception __hlt_channel_full = "ChannelFull";
__hlt_exception __hlt_channel_empty = "ChannelEmpty";
__hlt_exception __hlt_exception_unspecified = "Unspecified";
__hlt_exception __hlt_exception_decoding_error = "DecodingError";
__hlt_exception __hlt_exception_worker_thread_threw_exception = "WorkerThreadThrewException";
__hlt_exception __hlt_exception_internal_error = "InternalError";
__hlt_exception __hlt_exception_os_error = "OSError";
__hlt_exception __hlt_exception_overlay_not_attached = "OverlayNotAttached";
__hlt_exception __hlt_exception_index_error = "IndexError";
__hlt_exception __hlt_exception_underflow = "Underflow";
__hlt_exception __hlt_exception_invalid_iterator = "InvalidIterator";
__hlt_exception __hlt_exception_pattern_error = "PatternError";

// Reports a caught exception.
void __hlt_exception_print(__hlt_exception exception) 
{
    const char* name = (const char*)exception; 
    fprintf(stderr, "hilti exception: %s\n", name);
}

// Reports an uncaught exception.
void __hlt_exception_print_uncaught(__hlt_exception exception) 
{
    const char* name = (const char*)exception; 
    fprintf(stderr, "hilti: uncaught exception, %s\n", name);
}
