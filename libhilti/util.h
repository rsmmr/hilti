
#ifndef HLT_UTIL_H
#define HLT_UTIL_H

#include <stdint.h>

// #include "threading.h"

/// Sleeps for a given amount of nano-seconds.
///
/// nsecs: The time to sleep.
extern void   hlt_util_nanosleep(uint64_t nsecs);

/// Renders an unsigned integer as a string.
///
/// value: The value to render.
///
/// buf: The target buffer.
///
/// n: The size of the buffer. If not sufficient, the result is undefined.
///
/// base: The base to use. The function can handle bases <= 74.
///
/// zerofill: True if the result should be prefixed with leading zeros to
/// fill up the buffer completely.
///
/// Returns: The numbers of bytes written into the buffer, counting zeros
/// used for filling as well.
extern int    hlt_util_uitoa_n(uint64_t value, char* buf, int n, int base, int zerofill);

/// Returns the number of CPU core the host system has.
///
/// \todo Only supported on Linux right now.
extern int    hlt_util_number_of_cpus();

/// Returns the memory usage of the current process. This is a wrapper around
/// the \c getrusage currently.
extern size_t hlt_util_memory_usage();

/// Wrapper around libpcap's pthread_setcancelstate that only forwards the
/// call if we are currently shutting down the HILTI runtime's processing.
/// This is the only time when we need to ensure the state is indeed set
/// right.
///
// extern void hlt_pthread_setcancelstate(int state, int *oldstate);

/// Aborts execution immediately with a core dump.
void hlt_abort();

// This is a noop as long as we aren't shutting down.
// extern void hlt_pthread_setcancelstate(int state, int *oldstate);

/// Converts a 64-bit value from host-order to network order.
///
/// v: The value to convert.
extern uint64_t hlt_hton64(uint64_t v);

/// Converts a 32-bit value from host-order to network order.
///
/// v: The value to convert.
extern uint32_t hlt_hton32(uint32_t v);

/// Converts a 16-bit value from host-order to network order.
///
/// v: The value to convert.
extern uint16_t hlt_hton16(uint16_t v);

#endif
