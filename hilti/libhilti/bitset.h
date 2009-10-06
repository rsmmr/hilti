// $Id$
// 
// Support functions for HILTI's bitset data type.

#ifndef HILTI_BITSET_H
#define HILTI_BITSET_H

#include "exceptions.h"

extern hlt_string hlt_bitset_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt);
extern int64_t hlt_bitset_to_int64(const hlt_type_info* type, const void* obj, hlt_exception** expt);

#endif
