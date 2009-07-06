// $Id$

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "hilti_intern.h"

__hlt_string __hlt_port_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt)
{
    assert(type->type == __HLT_TYPE_PORT);
    
    __hlt_port port = *((__hlt_port *)obj);
    
    const char *proto = (port.proto == __hlt_port_tcp) ? "tcp" : "udp";
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%d/%s", port.port, proto);
    return __hlt_string_from_asciiz(buffer, excpt);
}

int64_t __hlt_port_to_int64(const __hlt_type_info* type, const void* obj, __hlt_exception* expt)
{
    assert(type->type == __HLT_TYPE_PORT);
    __hlt_port port = *((__hlt_port *)obj);
    return port.port;
}

