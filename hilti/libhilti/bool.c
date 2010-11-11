/* $Id$
 * 
 * Support functions HILTI's bool data type.
 * 
 */

#include "hilti.h"

static hlt_string_constant True = { 4, "True" };
static hlt_string_constant False = { 5, "False" };

hlt_string hlt_bool_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** exception, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_BOOL);
    return *((int8_t*)obj) ? &True : &False;
}

int64_t hlt_bool_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_BOOL);
    return *((int8_t*)obj) ? 1 : 0;
}


