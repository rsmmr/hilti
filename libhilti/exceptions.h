/// 
/// C-level support for HILTI execptions. Exception objects are garbage
/// collected.
///

#ifndef LIBHILTI_EXCEPTIONS_H
#define LIBHILTI_EXCEPTIONS_H

#include <assert.h>

// #include "continuation.h"
// #include "threading.h"
#include "memory_.h"
#include "context.h"

struct __hlt_type_info;

/// The type of an exception. Note that this must align with hlt.exception.type in libhilti.ll
typedef struct hlt_exception_type {
    const char* name;                       //< Name of the exception.
    struct hlt_exception_type* parent;      //< The type this one derives from.
    const struct __hlt_type_info* argtype; //< The type of the exception's argument, or 0 if no argument.
} hlt_exception_type;

/// A HILTI exception instance.
struct __hlt_exception {
    __hlt_gchdr __gchdr;
    hlt_exception_type* type;        //< The type of the exeception.
    hlt_fiber* fiber;                //< If the exeption is resumable, the fiber for doing so.
    void *arg;                       //< The argument of type ``type->argtype``, or 0 if none.
    const char* location;            //< A string describing the location where the exception was raised.
    hlt_vthread_id vid;              //< If a virtual thread raised the exception, it's ID.
};

/// Instantiates a new exception.
///
/// type: The type of the exception.
///
/// arg: The exception argument if the type takes one, or null if not.
///
/// location: An optional string describing a location associated with the
/// exception, or null of none.
///
/// Returns: The new exception. Note that this is a garbage collected value
/// that must be released with hlt_exception_unref().
extern hlt_exception* hlt_exception_new(hlt_exception_type* type, void* arg, const char* location);

/// Instantiates a new yield exception.
///
/// fiber: The fiber to resume later.
///
/// location: An optional string describing a location associated with the
/// exception, or null of none.
///
/// Returns: The new exception. Note that this is a garbage collected value
/// that must be released with hlt_exception_unref().
extern hlt_exception* hlt_exception_new_yield(hlt_fiber* fiber, const char* location);

/// Returns the exception's argument.
extern void* hlt_exception_arg(hlt_exception* excpt);

/// Returns the exception's fiber for resuming if set, or null if not.
extern hlt_fiber* __hlt_exception_fiber(hlt_exception* excpt);

/// Clears the exception's fiber field. Note that it doesn't destroy the fiber.
extern void __hlt_exception_clear_fiber(hlt_exception* excpt);

// extern hlt_exception* __hlt_exception_new_yield(hlt_continuation* cont, int32_t arg, const char* location);

/// Internal function that checks whether an exception instance matches a
/// given exception type or any of its base types. This function is used to
/// find the right \c catch handler. If type is null, return true.
extern int8_t __hlt_exception_match(hlt_exception*, hlt_exception_type* type);

/// Returns true if the given exception is a \a yield exception.
extern int8_t hlt_exception_is_yield(hlt_exception* excpt);

/// Returns true if the given exception is a \a termination exception.
extern int8_t hlt_exception_is_termination(hlt_exception* excpt);

/// Internal function that generates the output shown to the user when an
/// exception is not caught, and then aborts processing. This function is
/// intended for use outside of threads.
///
/// exception: The exception to print.
///
/// ctx: The current execution context.
extern void hlt_exception_print_uncaught_abort(hlt_exception* exception, hlt_execution_context* ctx);

/// Internal function that generates the output shown to the user when an
/// exception is not caught.  This function is intended for use outside of
/// threads.
///
/// exception: The exception to print.
///
/// ctx: The current execution context.
extern void hlt_exception_print_uncaught(hlt_exception* exception, hlt_execution_context* ctx);

/// Internal function that generates the output shown to the user when an
/// exception is not caught.  This function is intended for use when the
/// exception was thrown in a thread.
///
/// exception: The exception to print.
///
/// ctx: The current execution context.
extern void hlt_exception_print_uncaught_in_thread(hlt_exception* exception, hlt_execution_context* ctx);

/// Prints out an exception objcect to stderr.
///
/// exception: The exception to print.
///
/// ctx: The current execution context.
extern void hlt_exception_print(hlt_exception* exception, hlt_execution_context* ctx);

/// Converts an exception into a string.
///
/// Include: include-to-string-sig.txt
hlt_string hlt_exception_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);

#define __hlt_stringify2(arg) #arg
#define __hlt_stringify(arg) __hlt_stringify2(arg)

/// Macro to raise a HILTI exception from C code. It allocates a new
/// exception object and stores it at the given location. Note that the new
/// object is memory managed and needs to be unref'ed eventually.
///
/// This is a macro so that it can automatically add the current source
/// location to the exeption.
///
/// dst: A pointer to the location where to store the raised exception.
/// Often, this will be *expt* parameter passed as part of the C-HILTI
/// calling convention. It uses GC_ASSIGN for the assignement so if that
/// location already stores an exception, that will be unrefed first.
///
/// type: The type of the exception.
///
/// arg: The exception's argument if the type takes any, or null if not.
#define hlt_set_exception(dst, type, arg) __hlt_set_exception(dst, type, arg, __FILE__ ":" __hlt_stringify(__LINE__))

/// Internal function that raises a HILTI exception from C code. This should
/// not be used directly, but only via the hlt_set_exception() macro.
extern void __hlt_set_exception(hlt_exception** dst, hlt_exception_type* type, void* arg, const char* location);


/// \addtogroup predefined-exceptions
/// @{
///
/// Predefined HILTI exceptions.

/// Fall-back exception if nothing else is specified.
extern hlt_exception_type hlt_exception_unspecified;

/// No exception.
extern hlt_exception_type hlt_exception_none;

/// A division by zero has occured.
extern hlt_exception_type hlt_exception_division_by_zero;

/// A value looks different than expected.
extern hlt_exception_type hlt_exception_value_error;

/// A function received different arguments than it expected.
extern hlt_exception_type hlt_exception_wrong_arguments;

/// An memory allocation has failed due to resource exhaustion.
extern hlt_exception_type hlt_exception_out_of_memory;

/// An undefined value has been attempted to use.
extern hlt_exception_type hlt_exception_undefined_value;

/// An internal error that indicates a bug in HILTI.
extern hlt_exception_type hlt_exception_internal_error;

/// An operation could not be performed without blocking.
extern hlt_exception_type hlt_exception_would_block;

/// A worker thread threw an exception.
extern hlt_exception_type hlt_exception_uncaught_thread_exception;

/// An operation that depends on having threads was attempted even though we
/// aren't configured for threading.
extern hlt_exception_type hlt_exception_no_threading;

/// A system or libc function returned an unexpected error.
extern hlt_exception_type hlt_exception_os_error;

/// An overlay instruction has been used on a not yet attached overlay.
extern hlt_exception_type hlt_exception_overlay_not_attached;

/// An invalid container index.
extern hlt_exception_type hlt_exception_index_error;

/// A container item was attempted to read which isn't there.
extern hlt_exception_type hlt_exception_underflow;

/// An interator is used which is not valid for the operation.
extern hlt_exception_type hlt_exception_invalid_iterator;

/// An error in a regular expression.
extern hlt_exception_type hlt_exception_pattern_error;

/// Functionality not yet implemented.
extern hlt_exception_type hlt_exception_not_implemented;

/// Base class for all resumable exceptions.
extern hlt_exception_type hlt_exception_yield;

/// A resumable exception generated by the Yield statement to temporarily
/// suspend execution.
extern hlt_exception_type hlt_exception_yield;

/// Raised when debug.assert fails.
extern hlt_exception_type hlt_exception_assertion_error;

/// Raised when we encounter an unexpected null reference.
extern hlt_exception_type hlt_exception_null_reference;

/// Raised when a timer is associated with multiple timer managers.
extern hlt_exception_type hlt_exception_timer_already_scheduled;

/// Raised when a timer is must be scheduled for an operation but is not.
extern hlt_exception_type hlt_exception_timer_not_scheduled;

/// Raised when expiration is requested for a container with which no timer
/// manager has been associated.
extern hlt_exception_type hlt_exception_no_timer_manager;

/// Raised when a packet source cannot provide any further packets.
extern hlt_exception_type hlt_exception_iosrc_exhausted;

/// Raised when an I/O operation failed.
extern hlt_exception_type hlt_exception_io_error;

/// An incompatible profiler has been specified. 
extern hlt_exception_type hlt_exception_profiler_mismatch;

/// An unknown profiler has been specified.
extern hlt_exception_type hlt_exception_profiler_unknown;

/// A thread.schedule instruction was executed without a thread context being set.
extern hlt_exception_type hlt_exception_no_thread_context;

/// Converting a value from one type to another failed.
extern hlt_exception_type hlt_exception_conversion_error;

/// Raised by Hilti::terminate() to terminate exectuin.
extern hlt_exception_type hlt_exception_termination;


/// @}

#endif
