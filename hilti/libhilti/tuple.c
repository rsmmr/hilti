/* $Id$
 * 
 * Support functions HILTI's tuple data type.
 * 
 */

#include <stdio.h>

#include "hilti_intern.h"

static const __hlt_string prefix = { 1, "(" };
static const __hlt_string postfix = { 1, ")" };
static const __hlt_string separator = { 1, "," };

const __hlt_string* __hlt_tuple_fmt(const __hlt_type_info* type, void* (*obj[]), int32_t options, __hlt_exception* excpt)
{
    int i;
    const __hlt_string *s = __hlt_string_from_asciiz("", excpt);
    
    s = __hlt_string_concat(s, &prefix, excpt);
    if ( *excpt )
        return 0;
    
    for ( i = 0; i < type->num_params; i++ ) {
        const __hlt_string *t;
        __hlt_type_info** types = (__hlt_type_info**) &type->type_params;

        if ( types[i]->libhilti_fmt ) {
            t = (types[i]->libhilti_fmt)(types[i], (*obj)[i], 0, excpt);
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

