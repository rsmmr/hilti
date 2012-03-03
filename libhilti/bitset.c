//
// Support functions HILTI's bitset data type.
//

#include "bitset.h"
#include "string_.h"

#include <stdio.h>

typedef struct {
    uint8_t bit;
    const char* name;
} Label;

hlt_string hlt_bitset_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string sep = hlt_string_from_asciiz("|", excpt, ctx);

    assert(type->type == HLT_TYPE_BITSET);
    int64_t i = *((int64_t*)obj);

    hlt_string str = 0;
    Label *labels = (Label*) type->aux;

    while ( labels->name ) {
        if ( ! (i & (1 << labels->bit)) ) {
            ++labels;
            continue;
        }

        if ( ! str )
            str = hlt_string_from_asciiz(labels->name, excpt, ctx);

        else {
            hlt_string tmp = str;
            str = hlt_string_concat(str, sep, excpt, ctx);
            GC_DTOR(tmp, hlt_string);
            tmp = str;
            hlt_string tmp2 = hlt_string_from_asciiz(labels->name, excpt, ctx);
            str = hlt_string_concat(str, tmp2, excpt, ctx);
            GC_DTOR(tmp, hlt_string);
            GC_DTOR(tmp2, hlt_string);
        }

        ++labels;
    }

    GC_DTOR(sep, hlt_string);

    if ( str )
        return str;
    else
        return hlt_string_from_asciiz("<empty>", excpt, ctx);
}

int64_t hlt_bitset_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_BITSET);
    return *((int64_t*)obj);
}


