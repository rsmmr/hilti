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

#include "hilti.h"

hlt_exception hlt_exception_division_by_zero = "DivisionByZero";
hlt_exception hlt_exception_value_error = "ValueError";
hlt_exception hlt_exception_out_of_memory = "OutOfMemory";
hlt_exception hlt_exception_wrong_arguments = "WrongArguments";
hlt_exception hlt_exception_undefined_value = "UndefinedValue";
hlt_exception hlt_channel_full = "ChannelFull";
hlt_exception hlt_channel_empty = "ChannelEmpty";
hlt_exception hlt_exception_unspecified = "Unspecified";
hlt_exception hlt_exception_decoding_error = "DecodingError";
hlt_exception hlt_exception_worker_thread_threw_exception = "WorkerThreadThrewException";
hlt_exception hlt_exception_internal_error = "InternalError";
hlt_exception hlt_exception_os_error = "OSError";
hlt_exception hlt_exception_overlay_not_attached = "OverlayNotAttached";
hlt_exception hlt_exception_index_error = "IndexError";
hlt_exception hlt_exception_underflow = "Underflow";
hlt_exception hlt_exception_invalid_iterator = "InvalidIterator";
hlt_exception hlt_exception_pattern_error = "PatternError";

// Reports a caught exception.
void hlt_exception_print(hlt_exception exception) 
{
    const char* name = (const char*)exception; 
    fprintf(stderr, "hilti exception: %s\n", name);
}

// Reports an uncaught exception.
void hlt_exception_print_uncaught(hlt_exception exception) 
{
    const char* name = (const char*)exception; 
    fprintf(stderr, "hilti: uncaught exception, %s\n", name);
}
