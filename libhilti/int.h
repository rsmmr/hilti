///
/// Support functions for HILTI's integer data type.
///

#ifndef LIBHILTI_INT_H
#define LIBHILTI_INT_H

#include "types.h"

/// Implements integer exponentation. Note that both base and exponent are
/// unsigned.
///
/// base: The base.
///
/// exp: The exponent.
///
/// \hlt_c
extern uint64_t hlt_int_pow(uint64_t base, uint64_t exp, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a HILTI tuple into a HILTI string.
///
/// \hlt_to_string
extern hlt_string hlt_int_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a HILTI tuple into a HILTI string.
///
/// \hlt_to_int64
extern int64_t hlt_int_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);

#endif
