/**
 * Code for exchanging information between the compiler and the runtime.
 *
 */

#ifndef BRO_PLUGIN_HILTI_RUNTIME_INTERFACE_H
#define BRO_PLUGIN_HILTI_RUNTIME_INTERFACE_H

extern "C" {

#include <stdint.h>

class BroType;
class BroObj;

// The numerical value we use for enums' \c Undef value inside the
// corresponding Bro type definition.
static const int lib_bro_enum_undef_val = 9999999;

// XXX
uint64_t lib_bro_add_indexed_type(BroType* type);

// XXX
BroType* lib_bro_get_indexed_type(uint64_t idx);

// XXX Forward to libbro_object_mapping_unregister_bro, which has HILTI-C calling convention.
void lib_bro_object_mapping_unregister_bro(void* obj);

// XXX Forward to libbro_object_mapping_invalidate_bro, which has HILTI-C calling convention.
void lib_bro_object_mapping_invalidate_bro(void* obj);
}

#endif
