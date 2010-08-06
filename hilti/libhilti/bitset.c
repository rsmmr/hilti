/* $Id$
 * 
 * Support functions HILTI's bitset data type.
 * 
 */

#include "hilti.h"

#include <stdio.h>

static hlt_string_constant _sep = { 1, "|" };
static hlt_string_constant _none = { 7, "<empty>" };

hlt_string hlt_bitset_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_BITSET);
    int64_t i = *((int64_t*)obj);

    // Labels are stored as concatenated ASCIIZ. 
    const char *labels = (const char*) type->aux;
    
    hlt_string str = 0;
    int bit = 1;
    while ( *labels ) {
        if ( i & bit ) {
            if ( ! str )
                str = hlt_string_from_asciiz(labels, excpt, ctx);
            else {
                str = hlt_string_concat(str, &_sep, excpt, ctx);
                str = hlt_string_concat(str, hlt_string_from_asciiz(labels, excpt, ctx), excpt, ctx);
            }

        }
        bit <<= 1;
        while ( *labels++ );
    }
            
    return str ? str : &_none;
}

int64_t hlt_bitset_to_int64(const hlt_type_info* type, const void* obj, hlt_exception** expt)
{
    assert(type->type == HLT_TYPE_BITSET);
    return *((int64_t*)obj);
}


