//
// Support functions HILTI's enum data type.
//

#include <inttypes.h>
#include <stdio.h>

#include "enum.h"
#include "string_.h"

typedef struct {
    uint64_t value;
    const char* name;
} Label;

int64_t hlt_enum_value(hlt_enum e, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return e.value;
}

hlt_enum hlt_enum_unset(hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_enum unset = { HLT_ENUM_UNDEF, 0 };
    return unset;
}

int8_t hlt_enum_equal(hlt_enum e1, hlt_enum e2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( hlt_enum_undefined(e1) || hlt_enum_undefined(e2) )
        return hlt_enum_undefined(e1) && hlt_enum_undefined(e2);

    return e1.value == e2.value;
}

hlt_string hlt_enum_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_ENUM);

    hlt_enum i = *((hlt_enum *) obj);

    if ( hlt_enum_undefined(i) ) {
        if ( hlt_enum_has_val(i) ) {
            // FIXME: Come up with something nicer.
            char buffer[128];
            snprintf(buffer, 128, "Unknown-%" PRId64, i.value);
            return hlt_string_from_asciiz(buffer, excpt, ctx);
        }
        else
            return hlt_string_from_asciiz("Undef", excpt, ctx);
    }

    Label *labels = (Label*) type->aux;

    while ( labels->name ) {
        if ( labels->value == i.value )
            return hlt_string_from_asciiz(labels->name, excpt, ctx);

        ++labels;
    }

    // Cannot be reached if the enum struct is used correctly.
    assert(0);
    return 0;
}

int64_t hlt_enum_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_ENUM);
    hlt_enum i = *((hlt_enum *) obj);

    return hlt_enum_undefined(i) ? -1 : i.value;
}


