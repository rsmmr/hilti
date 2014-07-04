///
/// Support functions for HILTI's integer data type.
///

#ifndef LIBHILTI_INT_H
#define LIBHILTI_INT_H

#include "types.h"
#include "enum.h"

/// Implements integer exponentation. Note that both base and exponent are
/// unsigned.
///
/// base: The base.
///
/// exp: The exponent.
///
/// \hlt_c
extern uint64_t hlt_int_pow(uint64_t base, uint64_t exp, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts an integer from a given byte order to host byte order.
///
/// v: The integer to convert.
///
/// byte_order: The byte order that *v* is in.
///
/// n: Number of bytes valid in *v*.
///
/// Returns: v in host byte order.
extern int64_t hlt_int_to_host(int64_t v, hlt_enum byte_order, int64_t n, hlt_exception** excpt, hlt_execution_context* ctx);

/// Reverses the bytes in an integer.
///
/// v: The integer to convert.
///
/// n: Number of bytes valid in *v*.
///
/// Returns: Reversed *n* bytes of *v*.
extern int64_t hlt_int_flip(int64_t v, int64_t n, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts an integer from host byte order to a given byte order.
///
/// v: The integer to convert in host byte order.
///
/// byte_order: The byte order to convert *v* in.
///
/// n: Number of bytes valid in *v*.
///
/// Returns: v in specified byte order.
extern int64_t hlt_int_from_host(int64_t v, hlt_enum byte_order, int64_t n, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a HILTI tuple into a HILTI string.
///
/// \hlt_to_string
extern hlt_string hlt_int_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a HILTI tuple into a HILTI string.
///
/// \hlt_to_int64
extern int64_t hlt_int_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);

#endif
