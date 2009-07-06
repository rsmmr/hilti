/* $Id$
 * 
 * Support functions HILTI's tuple data type.
 * 
 */

#include <stdio.h>

#include "hilti_intern.h"

static __hlt_string_constant prefix = { 1, "(" };
static __hlt_string_constant postfix = { 1, ")" };
static __hlt_string_constant separator = { 1, "," };

__hlt_string __hlt_tuple_to_string(const __hlt_type_info* type, const char* obj, int32_t options, __hlt_exception* excpt)
{
    assert(type->type == __HLT_TYPE_TUPLE);
    
    int16_t* offsets = (int16_t *)type->aux;
    
    int i;
    __hlt_string s = __hlt_string_from_asciiz("", excpt);
    
    s = __hlt_string_concat(s, &prefix, excpt);
    if ( *excpt )
        return 0;
    
    __hlt_type_info** types = (__hlt_type_info**) &type->type_params;
    for ( i = 0; i < type->num_params; i++ ) {
        __hlt_string t;

        if ( types[i]->to_string ) {
            t = (types[i]->to_string)(types[i], obj + offsets[i] , 0, excpt);
            if ( *excpt )
                return 0;
        }
        else
            // No format function.
            t = __hlt_string_from_asciiz(types[i]->tag, excpt);
        
        s = __hlt_string_concat(s, t, excpt);
        if ( *excpt )
            return 0;
        
        if ( i < type->num_params - 1 ) {
            s = __hlt_string_concat(s, &separator, excpt);
            if ( *excpt )
                return 0;
        }
        
    }
    
    return __hlt_string_concat(s, &postfix, excpt);
}
