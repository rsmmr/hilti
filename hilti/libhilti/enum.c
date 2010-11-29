/* $Id$
 *
 * Support functions HILTI's bool data type.
 *
 */

#include "hilti.h"

#include <stdio.h>

typedef struct {
    uint64_t value;
    hlt_string name;
} Label;

hlt_enum hlt_enum_unset(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_enum unset = { 1, 0, 0 };
    return unset;
}

int8_t hlt_enum_equal(hlt_enum e1, hlt_enum e2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( e1.undefined || e2.undefined )
        return e1.undefined && e2.undefined;

    return e1.value == e2.value;
}

static hlt_string_constant undefined = { 5, "Undef" };

hlt_string hlt_enum_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_ENUM);

    hlt_enum i = *((hlt_enum *) obj);

    if ( i.undefined ) {
        if ( i.value_set ) {
            // FIXME: Come up with something nicer.
            char buffer[128];
            int len = snprintf(buffer, 128, "Unknown-%" PRId64, i.value);
            return hlt_string_from_asciiz(buffer, excpt, ctx);
        }
        else
            return &undefined;
    }

    Label *labels = (Label*) type->aux;

    while ( labels->name ) {
        if ( labels->value == i.value )
            return labels->name;

        ++labels;
    }

    // Cannot be reached if the enum struct is used correctly.
    assert(false);
    return 0;
}

int64_t hlt_enum_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_ENUM);
    hlt_enum i = *((hlt_enum *) obj);

    return i.undefined ? -1 : i.value;
}


