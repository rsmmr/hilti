/* $Id$
 * 
 * Support functions HILTI's tuple data type.
 * 
 */

#include <stdio.h>

#include "hilti.h"

static hlt_string_constant prefix = { 1, "(" };
static hlt_string_constant postfix = { 1, ")" };
static hlt_string_constant separator = { 1, "," };

hlt_string hlt_tuple_to_string(const hlt_type_info* type, const char* obj, int32_t options, hlt_exception* excpt)
{
    assert(type->type == HLT_TYPE_TUPLE);
    
    int16_t* offsets = (int16_t *)type->aux;
    
    int i;
    hlt_string s = hlt_string_from_asciiz("", excpt);
    
    s = hlt_string_concat(s, &prefix, excpt);
    if ( *excpt )
        return 0;
    
    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    for ( i = 0; i < type->num_params; i++ ) {
        hlt_string t;

        if ( types[i]->to_string ) {
            t = (types[i]->to_string)(types[i], obj + offsets[i] , 0, excpt);
            if ( *excpt )
                return 0;
        }
        else
            // No format function.
            t = hlt_string_from_asciiz(types[i]->tag, excpt);
        
        s = hlt_string_concat(s, t, excpt);
        if ( *excpt )
            return 0;
        
        if ( i < type->num_params - 1 ) {
            s = hlt_string_concat(s, &separator, excpt);
            if ( *excpt )
                return 0;
        }
        
    }
    
    return hlt_string_concat(s, &postfix, excpt);
}
