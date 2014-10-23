//
// Support functions HILTI's double data type.
//

#include <stdio.h>
#include <string.h>

#include "double.h"
#include "string_.h"

hlt_string hlt_double_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_DOUBLE);

    double val = *((double *)obj);

    // FIXME: This is just a hack for now. Rather than depending on snprintf,
    // we should code our own dtoa().
    char buffer[128];
    snprintf(buffer, 128, "%.6f", val);
    return hlt_string_from_asciiz(buffer, excpt, ctx);
}

double hlt_double_to_double(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_DOUBLE);
    double val = *((double *)obj);
    return val;
}
