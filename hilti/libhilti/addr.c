// $Id$

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "hilti_intern.h"

static inline int is_v4(const __hlt_addr addr)
{
    return addr.a1 == 0 && (addr.a2 & 0xffffffff00000000) == 0;
}

// 64-bit version
#define htonll(x) \
    ((((x) & 0xff00000000000000LL) >> 56) | \
    (((x) & 0x00ff000000000000LL) >> 40) | \
    (((x) & 0x0000ff0000000000LL) >> 24) | \
    (((x) & 0x000000ff00000000LL) >> 8) | \
    (((x) & 0x00000000ff000000LL) << 8) | \
    (((x) & 0x0000000000ff0000LL) << 24) | \
    (((x) & 0x000000000000ff00LL) << 40) | \
    (((x) & 0x00000000000000ffLL) << 56))

__hlt_string __hlt_addr_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt)
{
    assert(type->type == __HLT_TYPE_ADDR);
    
    __hlt_addr addr = *((__hlt_addr *)obj);
    
    char buffer[128];
    
    if ( is_v4(addr) ) {
        unsigned long a = (unsigned long)addr.a2;
        struct in_addr sa = { htonl(a) };
        
        if ( ! inet_ntop(AF_INET, &sa, buffer, 128) ) {
            *excpt = __hlt_exception_os_error;
            return 0;
        }
    }
        
    else {
        struct in6_addr sa;
        
        uint64_t a = htonll(addr.a1);
        memcpy(&sa, &a, 8);
        
        a = htonll(addr.a2);
        memcpy(((char*)&sa) + 8, &a, 8);
        
        if ( ! inet_ntop(AF_INET6, &sa, buffer, 128) ) {
            *excpt = __hlt_exception_os_error;
            return 0;
        }
    }
    
    return __hlt_string_from_asciiz(buffer, excpt);
}
        
