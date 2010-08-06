// $Id$
//
// FIXME: Most of this is copied from addr.c. We should merge the two.

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "hilti.h"

static inline int is_v4(const hlt_net net)
{
    return net.a1 == 0 && (net.a2 & 0xffffffff00000000) == 0 && net.len >= 96;
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

hlt_string hlt_net_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_NET);
    
    hlt_net net = *((hlt_net *)obj);

    int len;
    char buffer[128];
    
    if ( is_v4(net) ) {
        unsigned long a = (unsigned long)net.a2;
        struct in_addr sa = { htonl(a) };
        
        if ( ! inet_ntop(AF_INET, &sa, buffer, 128) ) {
            hlt_set_exception(excpt, &hlt_exception_os_error, 0);
            return 0;
        }
        
        len = net.len - 96;
    }
        
    else {
        struct in6_addr sa;
        
        uint64_t a = htonll(net.a1);
        memcpy(&sa, &a, 8);
        
        a = htonll(net.a2);
        memcpy(((char*)&sa) + 8, &a, 8);
        
        if ( ! inet_ntop(AF_INET6, &sa, buffer, 128) ) {
            hlt_set_exception(excpt, &hlt_exception_os_error, 0);
            return 0;
        }
        
        len = net.len;
    }
    
    char buffer2[6];
    snprintf(buffer2, sizeof(buffer2), "/%d", len);
    
    const hlt_string s1 = hlt_string_from_asciiz(buffer, excpt, ctx);
    const hlt_string s2 = hlt_string_from_asciiz(buffer2, excpt, ctx);
    
    return hlt_string_concat(s1, s2, excpt, ctx);
}
        
