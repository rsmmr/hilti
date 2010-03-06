/* $Id$
 * 
 * Support functions HILTI's caddr data type.
 * 
 */

#include "hilti.h"

hlt_string hlt_caddr_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** exception)
{
    assert(type->type == HLT_TYPE_CADDR);
    
    char buffer[32];
    snprintf(buffer, 32, "addr:%p", *((void**)obj));
    buffer[31] = '\0';
    
    return hlt_string_from_asciiz(buffer, exception);
}



