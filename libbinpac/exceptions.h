///
/// Predefined BinPAC exception.
///
///

#ifndef LIBBINPAC_EXCEPTIONS_H
#define LIBBINPAC_EXCEPTIONS_H

#include <libhilti.h>

/// Raised when a generated parser encounters an error in its input.
extern hlt_exception_type binpac_exception_parseerror;

/// Raised when a generated parser gets disabled in DFD mode.
extern hlt_exception_type binpac_exception_parserdisabled;

/// Raised when a ~~Filter encounters an error. 
extern hlt_exception_type binpac_exception_filtererror;

/// Raised when a ~~Filter not supported by the run-time system has been requested.
extern hlt_exception_type binpac_exception_filterunsupported;

#endif
