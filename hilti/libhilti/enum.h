// $Id$
// 
// Support functions for HILTI's XXX data type.

#ifndef HILTI_ENUM_H
#define HILTI_ENUM_H

#include "exceptions.h"

typedef struct
{
    int8_t undefined;
    int8_t value_set;
    int64_t value;
} hlt_enum;

extern hlt_string hlt_enum_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);
extern int64_t hlt_enum_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);

extern hlt_enum hlt_enum_unset(hlt_exception** excpt, hlt_execution_context* ctx);
extern int8_t hlt_enum_equal(hlt_enum e1, hlt_enum e2, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
