///
/// \addtogroup addr
///
/// @{
/// Functions for manipulating address objects.
///
/// A HILTI address is represented as an instance of ``hlt_addr``.

#ifndef HILTI_ADDR_H
#define HILTI_ADDR_H

#include "exceptions.h"
#include "rtti.h"
#include "string.h"
#include "context.h"

typedef struct __hlt_addr hlt_addr;

/// An instance of a HILTI ``addr``.
struct __hlt_addr {
    uint64_t a1; // The 8 more siginficant bytes.
    uint64_t a2; // The 8 less siginficant bytes.
};

/// Converts an address into a string representation. This function has the
/// generic ``hlt_*_to_string`` signature. 
///
/// type: The address' type information.
///
/// obj: A pointer to the address.
///
/// options: The standard conversion options.
///
/// excpt: &
/// ctx: & 
extern hlt_string hlt_addr_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts an ASCIIZ string into an address. The string can be either an
/// IPv4 or IPv6 address in their standard ASCII representations.
///
/// s: The string with the address.
///
/// excpt: &
///
/// Raises: ConversionError - If it can't parse the string.
extern hlt_addr hlt_addr_from_asciiz(const char* s, hlt_exception** excpt, hlt_execution_context* ctx);

/// @}

#endif

