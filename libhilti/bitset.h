///
/// \addtogroup bitset
///
/// @{
/// Functions for manipulating HILTI bitsets.
///
/// A bitset is represented as an ``uint64_t``, with bits in there set as
/// determined by the corresponding type definition. 

#ifndef LIBHILTI_BITSET_H
#define LIBHILTI_BITSET_H

#include "exceptions.h"

/// Converts a bitset into a string representation. This function has the
/// generic ``hlt_*_to_string`` signature. 
///
/// type: The bitset's type information.
///
/// obj: A pointer to the bitset.
///
/// options: The standard conversion options.
///
/// excpt: &
/// ctx: & 
extern hlt_string hlt_bitset_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a bitset into an integer representation. This function has the
/// generic ``hlt_*_to_int`` signature. 
///
/// type: The bitset's type information.
///
/// obj: A pointer to the bitset.
///
/// options: The standard conversion options.
///
/// excpt: &
/// ctx: & 
extern int64_t hlt_bitset_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);

/// @}

#endif
