// $Id$

#ifndef HILTI_ADDR_H
#define HILTI_ADDR_H

#include "exceptions.h"
#include "rtti.h"
#include "string.h"
#include "context.h"

typedef struct __hlt_addr hlt_addr;

struct __hlt_addr {
    uint64_t a1; // The 8 more siginficant bytes.
    uint64_t a2; // The 8 less siginficant bytes.
};

extern hlt_string hlt_addr_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
    
