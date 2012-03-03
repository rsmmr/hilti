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
#include "util.h"

static inline int is_v4(const hlt_net net)
{
    return net.a1 == 0 && (net.a2 & 0xffffffff00000000) == 0 && net.len >= 96;
}

hlt_string hlt_net_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_NET);

    hlt_net net = *((hlt_net *)obj);

    int len;
    char buffer[128];

    if ( is_v4(net) ) {
        unsigned long a = (unsigned long)net.a2;
        struct in_addr sa = { hlt_hton32(a) };

        if ( ! inet_ntop(AF_INET, &sa, buffer, 128) ) {
            hlt_set_exception(excpt, &hlt_exception_os_error, 0);
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
            hlt_set_exception(excpt, &hlt_exception_os_error, 0);
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

