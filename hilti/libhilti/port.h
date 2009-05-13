// $Id$

#ifndef PORT_H
#define PORT_H

static const uint8_t __hlt_port_tcp = 1;
static const uint8_t __hlt_port_udp = 2;

struct __hlt_port {
    uint16_t port;  // The port number. 
    uint8_t proto;  // The protocol per __hlt_port_*.
};

extern const __hlt_string* __hlt_port_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt);
extern int64_t __hlt_port_to_int64(const __hlt_type_info* type, const void* obj, __hlt_exception* expt);

#endif
    
