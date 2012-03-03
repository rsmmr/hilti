//
// Support functions HILTI's interval data type.
//

#ifndef HILTI_INTERVAL_H
#define HILTI_INTERVAL_H

#include "exceptions.h"

typedef uint64_t hlt_interval;

extern uint64_t hlt_interval_nsecs(hlt_interval t, hlt_exception** excpt, hlt_execution_context* ctx);

extern hlt_string hlt_interval_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);
extern int64_t hlt_interval_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);
extern double hlt_interval_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);

#endif
