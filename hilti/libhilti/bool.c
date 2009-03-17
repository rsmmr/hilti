/* $Id$
 * 
 * Support functions HILTI's bool data type.
 * 
 */

#include "hilti_intern.h"

static const __hlt_string True = { 4, "True" };
static const __hlt_string False = { 5, "False" };

const __hlt_string* __hlt_bool_to_string(const __hlt_type_info* type, void* obj, int32_t options, __hlt_exception* exception)
{
    assert(type->type == __HLT_TYPE_BOOL);
    return *((int8_t*)obj) ? &True : &False;
}

int64_t __hlt_bool_to_int64(const __hlt_type_info* type, void* obj, __hlt_exception* expt)
{
    assert(type->type == __HLT_TYPE_BOOL);
    return *((int8_t*)obj) ? 1 : 0;
}


