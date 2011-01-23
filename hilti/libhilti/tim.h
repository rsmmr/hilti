// $Id$
//
// Support functions for HILTI's TIME data type.

#ifndef HILTI_TIME_H
#define HILTI_TIME_H

#include "hilti.h"

typedef uint64_t hlt_time;

#define HLT_TIME_UNSET 0xffffffffffffffff // Marker for a non-set time.

extern hlt_time hlt_time_value(uint64_t secs, uint64_t nsecs);

extern hlt_string hlt_time_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);
extern int64_t hlt_time_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);
extern double hlt_time_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);

#endif
