///
/// Support functions for HILTI's tuple data type.
///

#ifndef LIBHILTI_TUPLE_H
#define LIBHILTI_TUPLE_H

#include "types.h"

/// Converts a HILTI tuple into a HILTI string.
///
/// \hlt_to_string
extern hlt_string hlt_tuple_to_string(const hlt_type_info* type, void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx);

#endif


