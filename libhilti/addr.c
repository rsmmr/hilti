
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "addr.h"
#include "string_.h"
#include "hutil.h"

static inline int is_v4(const hlt_addr addr)
{
    return addr.a1 == 0 && (addr.a2 & 0xffffffff00000000) == 0;
}

hlt_string hlt_addr_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_ADDR);

    hlt_addr addr = *((hlt_addr *)obj);

    if ( is_v4(addr) ) {
        unsigned long a = (unsigned long)addr.a2;
        struct in_addr sa = { hlt_hton32(a) };

        char buffer[INET_ADDRSTRLEN];

        if ( ! inet_ntop(AF_INET, &sa, buffer, sizeof(buffer)) ) {
            hlt_set_exception(excpt, &hlt_exception_os_error, 0);
            return 0;
        }

        return hlt_string_from_asciiz(buffer, excpt, ctx);
    }

    else {
        struct in6_addr sa;

        uint64_t a = hlt_hton64(addr.a1);
        memcpy(&sa, &a, 8);

        a = hlt_hton64(addr.a2);
        memcpy(((char*)&sa) + 8, &a, 8);

        char buffer[INET6_ADDRSTRLEN];

        if ( ! inet_ntop(AF_INET6, &sa, buffer, sizeof(buffer)) ) {
            hlt_set_exception(excpt, &hlt_exception_os_error, 0);
            return 0;
        }

        return hlt_string_from_asciiz(buffer, excpt, ctx);
    }
}

int8_t hlt_addr_is_v6(hlt_addr addr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return ! is_v4(addr);
}

struct in_addr hlt_addr_to_in4(hlt_addr addr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! is_v4(addr) ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0);
        struct in_addr sa;
        return sa;
    }

    unsigned long a = (unsigned long)addr.a2;
    struct in_addr sa = { hlt_hton32(a) };
    return sa;
}

struct in6_addr hlt_addr_to_in6(hlt_addr addr, hlt_exception** excpt, hlt_execution_context* ctx)
{
    struct in6_addr sa;

    uint64_t a = hlt_hton64(addr.a1);
    memcpy(&sa, &a, 8);

    a = hlt_hton64(addr.a2);
    memcpy(((char*)&sa) + 8, &a, 8);

    return sa;
}

hlt_addr hlt_addr_from_in4(struct in_addr in, hlt_exception** excpt, hlt_execution_context* ctx)
    {
    hlt_addr addr = { 0, 0 };
    addr.a2 = hlt_ntoh32(in.s_addr);
    return addr;
    }

hlt_addr hlt_addr_from_in6(struct in6_addr in, hlt_exception** excpt, hlt_execution_context* ctx)
    {
    uint64_t a1;
    memcpy(&a1, &in, 8);

    uint64_t a2;
    memcpy(&a2, ((char*)&in) + 8, 8);

    hlt_addr a = { hlt_ntoh64(a1), hlt_ntoh64(a2) };
    return a;
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
        a.a1 = hlt_ntoh64(a.a1);

        memcpy(&a.a2, ((char*)&in6) + 8, 8);
        a.a2 = hlt_ntoh64(a.a2);
    }

    else {
        if ( inet_pton(AF_INET, s, &in) <= 0 )
            goto error;

        a.a1 = 0;
        a.a2 = hlt_ntoh32(in.s_addr);
    }

    return a;

error:
    hlt_set_exception(excpt, &hlt_exception_conversion_error, 0);
    return a;
}

