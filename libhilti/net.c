//
// Support functions HILTI's net data type.
//
// FIXME: Most of this is copied from addr.c. We should merge the two.

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "net.h"
#include "string_.h"
#include "hutil.h"

static inline int is_v4(const hlt_net net)
{
    return net.a1 == 0 && (net.a2 & 0xffffffff00000000) == 0 && net.len >= 96;
}

int8_t hlt_net_is_v6(hlt_net net, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return ! is_v4(net);
}

struct in_addr hlt_net_to_in4(hlt_net net, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( ! is_v4(net) ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        struct in_addr sa;
        return sa;
    }

    unsigned long a = (unsigned long)net.a2;
    struct in_addr sa = { hlt_hton32(a) };
    return sa;
}

struct in6_addr hlt_net_to_in6(hlt_net net, hlt_exception** excpt, hlt_execution_context* ctx)
{
    struct in6_addr sa;

    uint64_t a = hlt_hton64(net.a1);
    memcpy(&sa, &a, 8);

    a = hlt_hton64(net.a2);
    memcpy(((char*)&sa) + 8, &a, 8);

    return sa;
}

uint8_t hlt_net_length(hlt_net net, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return net.len;
}

hlt_net hlt_net_from_in4(struct in_addr in, uint8_t length, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_net net = { 0, 0, length };
    net.a2 = hlt_ntoh32(in.s_addr);
    return net;
}

hlt_net hlt_net_from_in6(struct in6_addr in, uint8_t length, hlt_exception** excpt, hlt_execution_context* ctx)
{
    uint64_t a1;
    memcpy(&a1, &in, 8);

    uint64_t a2;
    memcpy(&a2, ((char*)&in) + 8, 8);

    hlt_net net = { hlt_ntoh64(a1), hlt_ntoh64(a1), length };
    return net;
}

hlt_string hlt_net_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_NET);

    hlt_net net = *((hlt_net *)obj);

    int len;
    char buffer[128];

    if ( is_v4(net) ) {
        unsigned long a = (unsigned long)net.a2;
        struct in_addr sa = { hlt_hton32(a) };

        if ( ! inet_ntop(AF_INET, &sa, buffer, 128) ) {
            hlt_set_exception(excpt, &hlt_exception_os_error, 0, ctx);
            return 0;
        }

        len = net.len - 96;
    }

    else {
        struct in6_addr sa;

        uint64_t a = hlt_hton64(net.a1);
        memcpy(&sa, &a, 8);

        a = hlt_hton64(net.a2);
        memcpy(((char*)&sa) + 8, &a, 8);

        if ( ! inet_ntop(AF_INET6, &sa, buffer, 128) ) {
            hlt_set_exception(excpt, &hlt_exception_os_error, 0, ctx);
            return 0;
        }

        len = net.len;
    }

    char buffer2[6];
    snprintf(buffer2, sizeof(buffer2), "/%d", len);

    len = strlen(buffer);
    strncpy(buffer + len, buffer2, sizeof(buffer) - len);

    return hlt_string_from_asciiz(buffer, excpt, ctx);
}

