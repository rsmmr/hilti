
#ifndef HLT_UTIL_H
#define HLT_UTIL_H

#include <stdint.h>

// #include "threading.h"

extern void   hlt_util_nanosleep(uint64_t nsecs);
extern int    hlt_util_uitoa_n(uint64_t value, char* buf, int n, int base, int zerofill);
extern int    hlt_util_number_of_cpus();
extern size_t hlt_util_memory_usage();

// This is a noop as long as we aren't shutting down.
// extern void hlt_pthread_setcancelstate(int state, int *oldstate);

#endif
