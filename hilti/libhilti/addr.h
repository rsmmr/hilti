// $Id$

#ifndef ADDR_H
#define ADDR_H

struct __hlt_addr {
    uint64_t a1; // The 8 more siginficant bytes.
    uint64_t a2; // The 8 less siginficant bytes.
};

extern __hlt_string __hlt_addr_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt);

#endif
    
