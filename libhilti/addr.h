///
/// \addtogroup addr
///
/// @{
/// Functions for manipulating address objects.
///
/// A HILTI address is represented as an instance of ``hlt_addr``.

#ifndef LIBHILTI_ADDR_H
#define LIBHILTI_ADDR_H

#include <netinet/in.h>

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
extern hlt_string hlt_addr_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns true for an address of IPv6 family and false for IPv4.
///
/// addr: The address to check.
///
/// excpt: &
/// ctx: &
extern int8_t hlt_addr_is_v6(hlt_addr addr, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts an address into an \c in_addr representation. This will throw a
/// \a ValueError exception for IPv6 addresses.
///
/// addr: The address to convert.
///
/// excpt: &
/// ctx: & 
extern struct in_addr hlt_addr_to_in4(hlt_addr addr, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts an address into an \c in6_addr representation.
///
/// addr: The address to convert.
///
/// excpt: &
/// ctx: & 
extern struct in6_addr hlt_addr_to_in6(hlt_addr addr, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a IPv4 \c in_addr into an address.
///
/// in: The value to convert.
///
/// excpt: &
extern hlt_addr hlt_addr_from_in4(struct in_addr in, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a IPv6 \c in6_addr into an address.
///
/// in: The value to convert.
///
/// excpt: &
extern hlt_addr hlt_addr_from_in6(struct in6_addr in, hlt_exception** excpt, hlt_execution_context* ctx);

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

