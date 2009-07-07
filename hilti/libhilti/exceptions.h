#ifndef HILTI_EXCEPTIONS_H
#define HILTI_EXCEPTIONS_H

    // %doc-hlt_exception-start
typedef const char* hlt_exception;
    // %doc-hlt_exception-end

///////////////////////////////////////////////////////////////////////////////
// Predefined exceptions.
///////////////////////////////////////////////////////////////////////////////

    // %doc-std-exceptions-start
// A division by zero has occured.
extern hlt_exception hlt_exception_division_by_zero;

// A value looks different than expected. 
extern hlt_exception hlt_exception_value_error;

// A function received different arguments than it expected. 
extern hlt_exception hlt_exception_wrong_arguments;

// An memory allocation has failed due to resource exhaustion.
extern hlt_exception hlt_exception_out_of_memory;

// An undefined value has been attempted to use. 
extern hlt_exception hlt_exception_undefined_value;

// An internal error that indicates a bug in HILTI.
extern hlt_exception hlt_exception_internal_error;

// A write operation on a full channel has been attempted.
extern hlt_exception hlt_channel_full;

// A read operation on an empty channel has been attempted.
extern hlt_exception hlt_channel_empty;

// A worker thread threw an exception.
extern hlt_exception hlt_exception_worker_thread_threw_exception;

// A system or libc function returned an unexpected error.
extern hlt_exception hlt_exception_os_error;

// An overlay instruction has been used on a not yet attached overlay.
extern hlt_exception hlt_exception_overlay_not_attached;

// An invalid container index.
extern hlt_exception hlt_exception_index_error;

// A container item was attempted to read which isn't there. 
extern hlt_exception hlt_exception_underflow;

// An interator is used which is not valid for the operation.
extern hlt_exception hlt_exception_invalid_iterator;

// An error in a regular expression.
extern hlt_exception hlt_exception_pattern_error;

// Fall-back exception if nothing else is specified.
extern hlt_exception hlt_exception_unspecified;

///////////////////////////////////////////////////////////////////////////////
// Exception-related functions.
///////////////////////////////////////////////////////////////////////////////

extern void hlt_exception_print(hlt_exception exception); 
extern void hlt_exception_print_uncaught(hlt_exception exception); 

#endif
