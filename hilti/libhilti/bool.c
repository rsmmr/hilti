/* $Id$
 * 
 * Support functions HILTI's bool data type.
 * 
 */

#include "hilti_intern.h"

static const __hlt_string True = { 4, "True" };
static const __hlt_string False = { 4, "False" };

const __hlt_string* __hlt_bool_fmt(const __hlt_type_info* type, void* obj, int32_t options, __hlt_exception* exception)
{
    return *((int8_t*)obj) ? &True : &False;
}

