// $Id$

#ifndef HILTI_NET_H
#define HILTI_NET_H

#include "exceptions.h"

typedef struct __hlt_net hlt_net;

struct __hlt_net {
    uint64_t a1; // The 8 more siginficant bytes of the mask
    uint64_t a2; // The 8 less siginficant bytes of the mask
    uint8_t len; // The length of the mask.
};

extern hlt_string hlt_net_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);

#endif

