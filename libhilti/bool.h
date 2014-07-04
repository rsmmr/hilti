/// \addtogroup string
/// @{
/// Functions for manipulating boolean values.

#ifndef LIBHILTI_BOOL_H
#define LIBHILTI_BOOL_H

#include "exceptions.h"

extern hlt_string hlt_bool_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx);
extern int64_t hlt_bool_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);

#endif

/// @}


