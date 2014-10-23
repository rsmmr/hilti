///
/// Support functions for HILTI's union data type.
///

#ifndef LIBHILTI_UNION_H
#define LIBHILTI_UNION_H

#include "types.h"

struct __hlt_union;

typedef struct __hlt_union hlt_union;

/// Converts a HILTI union into a HILTI string.
///
/// \hlt_to_string
extern hlt_string hlt_union_to_string(const hlt_type_info* type, void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx);

#endif


