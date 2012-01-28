///
/// Support functions for HILTI's tuple data type.
///

#ifndef HILTI_TUPLE_H
#define HILTI_TUPLE_H

#include "types.h"

/// Converts a HILTI tuple into a HILTI string.
///
/// \hlt_to_string
extern hlt_string hlt_tuple_to_string(const hlt_type_info* type, void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);

#endif


