// $Id$
// 
// Support functions for HILTI's double data type.

#ifndef HILTI_DOUBLE_H
#define HILTI_DOUBLE_H

#include "exceptions.h"

extern hlt_string hlt_double_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);
extern double hlt_double_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);

#endif
