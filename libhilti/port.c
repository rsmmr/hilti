//
// Support functions HILTI's port data type.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "port.h"
#include "string_.h"

hlt_string hlt_port_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_PORT);

    hlt_port port = *((hlt_port *)obj);

    const char* proto = 0;

    switch ( port.proto ) {
     case HLT_PORT_TCP:
        proto = "tcp";
        break;

     case HLT_PORT_UDP:
        proto = "udp";
        break;

     case HLT_PORT_ICMP:
        proto = "icmp";
        break;

     default:
        proto = "<unknown-protocol>";
        break;
    }

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%d/%s", port.port, proto);
    return hlt_string_from_asciiz(buffer, excpt, ctx);
}

int64_t hlt_port_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_PORT);
    hlt_port port = *((hlt_port *)obj);
    return port.port;
}

hlt_port hlt_port_from_asciiz(const char* s, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_port p;
    const char *t = s;

    while ( *t && isdigit(*t) )
        ++t;

    if ( s == t || *t != '/' )
        goto error;

    if ( strcmp(t, "/tcp") == 0 )
        p.proto = HLT_PORT_TCP;

    else if ( strcmp(t, "/udp") == 0 )
        p.proto = HLT_PORT_UDP;

    else if ( strcmp(t, "/icmp") == 0 )
        p.proto = HLT_PORT_ICMP;

    else
        goto error;

    p.port = atoi(s);
    return p;

error:
    hlt_set_exception(excpt, &hlt_exception_conversion_error, 0, ctx);
    return p;
}

