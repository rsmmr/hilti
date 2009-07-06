/* $Id$
 * 
 * Support functions HILTI's bool data type.
 * 
 */

#include "hilti_intern.h"

#include <stdio.h>

__hlt_string __hlt_enum_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt)
{
    assert(type->type == __HLT_TYPE_ENUM);
    int i = *((int8_t*)obj);
    
    const char *labels = (const char*) type->aux;
    
    while ( i-- ) 
        // Labels are stored as concatenated ASCIIZ. 
        while ( *labels++ );
        
    return __hlt_string_from_asciiz(labels, excpt);
}

int64_t __hlt_enum_to_int64(const __hlt_type_info* type, const void* obj, __hlt_exception* expt)
{
    assert(type->type == __HLT_TYPE_ENUM);
    return *((int8_t*)obj);
}


