///
/// Declares functions that the HILTI linker generates.
///

#ifndef LIBHILTI_WEAK_H
#define LIBHILTI_WEAK_H

#include <stdint.h>

extern void __hlt_modules_init(void* ctx);
extern void __hlt_globals_init(void* ctx);
extern void __hlt_globals_dtor(void* ctx);
extern uint64_t __hlt_globals_size() __attribute__((weak));

#endif
