// $Id$
//
// Support functions for HILTI's integer data type.

#ifndef HILTI_INT_H
#define HILTI_INT_H

#include "exceptions.h"

extern uint64_t hlt_int_pow(uint64_t base, uint64_t exp, hlt_exception** excpt, hlt_execution_context* ctx);
extern hlt_string hlt_int_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);
extern int64_t hlt_int_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);

#endif
