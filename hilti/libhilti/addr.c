// $Id$

#include "hilti.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

static inline int is_v4(const hlt_addr addr)
{
    return addr.a1 == 0 && (addr.a2 & 0xffffffff00000000) == 0;
}

// 64-bit versions
#define _flip(x) \
    ((((x) & 0xff00000000000000LL) >> 56) | \
    (((x) & 0x00ff000000000000LL) >> 40) | \
    (((x) & 0x0000ff0000000000LL) >> 24) | \
    (((x) & 0x000000ff00000000LL) >> 8) | \
    (((x) & 0x00000000ff000000LL) << 8) | \
    (((x) & 0x0000000000ff0000LL) << 24) | \
    (((x) & 0x000000000000ff00LL) << 40) | \
    (((x) & 0x00000000000000ffLL) << 56))

#if ! __BIG_ENDIAN__
# define ntohll(x) _flip(x)
# define htonll(x) _flip(x)
#else
# define ntohll(x) (x)
# define htonll(x) (x)
#endif

hlt_string hlt_addr_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_ADDR);

    hlt_addr addr = *((hlt_addr *)obj);

    char buffer[128];

    if ( is_v4(addr) ) {
        unsigned long a = (unsigned long)addr.a2;
        struct in_addr sa = { htonl(a) };

        if ( ! inet_ntop(AF_INET, &sa, buffer, 128) ) {
            hlt_set_exception(excpt, &hlt_exception_os_error, 0);
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
            hlt_set_exception(excpt, &hlt_exception_os_error, 0);
            return 0;
        }
    }

    return hlt_string_from_asciiz(buffer, excpt, ctx);
}

hlt_addr hlt_addr_from_asciiz(const char* s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_addr a;
    struct in_addr in;
    struct in6_addr in6;

    // We need to guess whether it's a IPv4 or IPv6 address. If there's a
    // colon in there, it's the latter.
    if ( strchr(s, ':') ) {
        if ( inet_pton(AF_INET6, s, &in6) <= 0 )
            goto error;

        memcpy(&a.a1, &in6, 8);
        a.a1 = ntohll(a.a1);

        memcpy(&a.a2, ((char*)&in6) + 8, 8);
        a.a2 = ntohll(a.a2);
    }

    else {
        if ( inet_pton(AF_INET, s, &in) <= 0 )
            goto error;

        a.a1 = 0;
        a.a2 = ntohl(in.s_addr);
    }

    return a;

error:
    hlt_set_exception(excpt, &hlt_exception_conversion_error, 0);
    return a;
}

