//
// Support functions HILTI's port data type.
//

#ifndef LIBHILTI_PORT_H
#define LIBHILTI_PORT_H

#include "exceptions.h"

static const uint8_t HLT_PORT_TCP = 1;
static const uint8_t HLT_PORT_UDP = 2;

typedef struct __hlt_port hlt_port;

// We use __packed__ struct so that we don't end up with unitialized bytes
// that would require a dedicated hash function.
struct __hlt_port {
    uint16_t port;  // The port number.
    uint8_t proto;  // The protocol per HLT_PORT_*.
} __attribute__((__packed__));

extern hlt_string hlt_port_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);
extern int64_t hlt_port_to_int64(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** expt, hlt_execution_context* ctx);

// Parses strings of the form "123/tcp" and "345/udp".
extern hlt_port hlt_port_from_asciiz(const char* str, hlt_exception** excpt, hlt_execution_context* ctx);

#endif

