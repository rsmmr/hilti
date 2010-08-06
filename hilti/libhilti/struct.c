/* $Id$
 * 
 * Support functions HILTI's struct data type.
 * 
 */

#include <stdio.h>

#include "hilti.h"
#include "str.h"

static hlt_string_constant prefix = { 1, "<" };
static hlt_string_constant postfix = { 1, ">" };
static hlt_string_constant separator = { 2, ", " };

hlt_string hlt_struct_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_STRUCT);
    
    int16_t* offsets = (int16_t *)type->aux;

    obj = *((const char**)obj);

    uint32_t mask = *((uint32_t*)obj);
    
    int i;
    hlt_string s = hlt_string_from_asciiz("", excpt, ctx);
    
    s = hlt_string_concat(s, &prefix, excpt, ctx);
    if ( *excpt )
        return 0;
    
    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    for ( i = 0; i < type->num_params; i++ ) {
        hlt_string t;
        
        uint32_t is_set = (mask & (1 << i));
        
        if ( ! is_set )
            t = hlt_string_from_asciiz("(not set)", excpt, ctx);
        
        else if ( types[i]->to_string ) {
            t = (types[i]->to_string)(types[i], obj + offsets[i] , 0, excpt, ctx);
            if ( *excpt )
                return 0;
        }
        else
            // No format function.
            t = hlt_string_from_asciiz(types[i]->tag, excpt, ctx);
        
        s = hlt_string_concat(s, t, excpt, ctx);
        if ( *excpt )
            return 0;
        
        if ( i < type->num_params - 1 ) {
            s = hlt_string_concat(s, &separator, excpt, ctx);
            if ( *excpt )
                return 0;
        }
        
    }
    
    return hlt_string_concat(s, &postfix, excpt, ctx);
}
