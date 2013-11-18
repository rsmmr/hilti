//
// Support functions HILTI's net data type.
//

#ifndef LIBHILTI_NET_H
#define LIBHILTI_NET_H

#include "exceptions.h"

typedef struct __hlt_net hlt_net;

struct __hlt_net {
    uint64_t a1; // The 8 more siginficant bytes of the mask
    uint64_t a2; // The 8 less siginficant bytes of the mask
    uint8_t len; // The length of the mask.
};

/// Returns true for a network of IPv6 family and false for IPv4.
///
/// net: The network to check.
///
/// excpt: &
/// ctx: &
extern int8_t hlt_net_is_v6(hlt_net net, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a network's prefix into an \c in_addr representation. This will
/// throw a \a ValueError exception for IPv6 addresses.
///
/// net: The network to convert.
///
/// excpt: &
/// ctx: & 
extern struct in_addr hlt_net_to_in4(hlt_net net, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a network's prefix into an \c in6_addr representation.
///
/// net: The network to convert.
///
/// excpt: &
/// ctx: & 
extern struct in6_addr hlt_net_to_in6(hlt_net net, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the length's of a network's prefix.
///
/// net: The network to return the length for.
///
/// excpt: &
/// ctx: & 
extern uint8_t hlt_net_length(hlt_net net, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a IPv4 \c in_addr into a network.
///
/// in: The value to convert.
///
/// len: The length of the network.
///
/// excpt: &
extern hlt_net hlt_net_from_in4(struct in_addr in, uint8_t length, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a IPv6 \c in6_addr into a network.
///
/// in: The value to convert.
///
/// len: The length of the network.
///
/// excpt: &
extern hlt_net hlt_net_from_in6(struct in6_addr in, uint8_t length, hlt_exception** excpt, hlt_execution_context* ctx);

/// XXX
extern hlt_string hlt_net_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);

#endif

