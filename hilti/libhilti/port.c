// $Id$

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "hilti.h"

hlt_string hlt_port_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_PORT);

    hlt_port port = *((hlt_port *)obj);

    const char *proto = (port.proto == hlt_port_tcp) ? "tcp" : "udp";
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

