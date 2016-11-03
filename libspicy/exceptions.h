///
/// Predefined Spicy exception.
///
///

#ifndef LIBSPICY_EXCEPTIONS_H
#define LIBSPICY_EXCEPTIONS_H

#include <libhilti.h>

/// Raised when a generated parser encounters an error in its input.
extern hlt_exception_type spicy_exception_parseerror;

/// Raised when a generated parser gets disabled in DFD mode.
extern hlt_exception_type spicy_exception_parserdisabled;

/// Raised when a ~~Filter encounters an error.
extern hlt_exception_type spicy_exception_filtererror;

/// Raised when a ~~Filter not supported by the run-time system has been requested.
extern hlt_exception_type spicy_exception_filterunsupported;

/// Raised when a value is used that wasn't expected or valid.
extern hlt_exception_type spicy_exception_valueerror;

/// Raised when a feature is used that's not yet implemented.
extern hlt_exception_type spicy_exception_notimplemented;

#endif
