/* $Id$
 *
 * Support functions HILTI's bitset data type.
 *
 */

#include "hilti.h"

#include <stdio.h>

typedef struct {
    uint8_t bit;
    hlt_string name;
} Label;


static hlt_string_constant _sep = { 1, "|" };
static hlt_string_constant _none = { 7, "<empty>" };

hlt_string hlt_bitset_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_BITSET);
    int64_t i = *((int64_t*)obj);

    hlt_string str = 0;
    Label *labels = (Label*) type->aux;

    while ( labels->name ) {
        if ( i & (1 << labels->bit) ) {
            if ( ! str )
                str = labels->name;
            else {
                str = hlt_string_concat(str, &_sep, excpt, ctx);
                str = hlt_string_concat(str, labels->name, excpt, ctx);
            }

        }

        ++labels;
    }

    return str ? str : &_none;
}

int64_t hlt_bitset_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt)
{
    assert(type->type == HLT_TYPE_BITSET);
    return *((int64_t*)obj);
}


