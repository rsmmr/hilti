// $Id$

#ifndef NET_H
#define NET_H

struct __hlt_net {
    uint64_t a1; // The 8 more siginficant bytes of the mask
    uint64_t a2; // The 8 less siginficant bytes of the mask
    uint8_t len; // The length of the mask. 
};

extern const __hlt_string* __hlt_net_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt);

#endif
    
