#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

///////////////////////////////////////////////////////////////////////////////
// Predefined exceptions.
///////////////////////////////////////////////////////////////////////////////

    // %doc-std-exceptions-start
// A division by zero has occured.
extern __hlt_exception __hlt_exception_division_by_zero;

// A value looks different than expected. 
extern __hlt_exception __hlt_exception_value_error;

// A function received different arguments than it expected. 
extern __hlt_exception __hlt_exception_wrong_arguments;

// An memory allocation has failed due to resource exhaustion.
extern __hlt_exception __hlt_exception_out_of_memory;

// An undefined value has been attempted to use. 
extern __hlt_exception __hlt_exception_undefined_value;

// An internal error that indicates a bug in HILTI.
extern __hlt_exception __hlt_exception_internal_error;

// A write operation on a full channel has been attempted.
extern __hlt_exception __hlt_channel_full;

// A read operation on an empty channel has been attempted.
extern __hlt_exception __hlt_channel_empty;

// A worker thread threw an exception.
extern __hlt_exception __hlt_exception_worker_thread_threw_exception;

// Fall-back exception if nothing else is specified.
extern __hlt_exception __hlt_exception_unspecified;

///////////////////////////////////////////////////////////////////////////////
// Exception-related functions.
///////////////////////////////////////////////////////////////////////////////

extern void __hlt_exception_print_uncaught(__hlt_exception exception); 

#endif
