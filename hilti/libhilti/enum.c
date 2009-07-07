/* $Id$
 * 
 * Support functions HILTI's bool data type.
 * 
 */

#include "hilti.h"

#include <stdio.h>

hlt_string hlt_enum_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception* excpt)
{
    assert(type->type == HLT_TYPE_ENUM);
    int i = *((int8_t*)obj);
    
    const char *labels = (const char*) type->aux;
    
    while ( i-- ) 
        // Labels are stored as concatenated ASCIIZ. 
        while ( *labels++ );
        
    return hlt_string_from_asciiz(labels, excpt);
}

int64_t hlt_enum_to_int64(const hlt_type_info* type, const void* obj, hlt_exception* expt)
{
    assert(type->type == HLT_TYPE_ENUM);
    return *((int8_t*)obj);
}


