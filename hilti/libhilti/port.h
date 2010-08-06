// $Id$

#ifndef HILTI_PORT_H
#define HILTI_PORT_H

#include "exceptions.h"

static const uint8_t hlt_port_tcp = 1;
static const uint8_t hlt_port_udp = 2;

typedef struct __hlt_port hlt_port;

struct __hlt_port {
    uint16_t port;  // The port number. 
    uint8_t proto;  // The protocol per hlt_port_*.
};

extern hlt_string hlt_port_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);
extern int64_t hlt_port_to_int64(const hlt_type_info* type, const void* obj, hlt_exception** expt, hlt_execution_context* ctx);

#endif
    
