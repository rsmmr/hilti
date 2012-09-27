///
/// Debugging output infrastructure.
///

#ifndef LIBHILTI_DEBUG_H
#define LIBHILTI_DEBUG_H

#include "types.h"

#ifdef DEBUG
# define DBG_LOG(args...) __hlt_debug_printf_internal(args)
#else
# define DBG_LOG(args...)
#endif

extern void hlt_debug_printf(hlt_string stream, hlt_string fmt, const hlt_type_info* type, const char* tuple, hlt_exception** excpt, hlt_execution_context* ctx);

/// Initializes libhilti's debugging subsystem if compiled in. If not, this
/// is just a no-op. The function is called from hlt_init().
extern void __hlt_debug_init();

/// Terminates libhilti's debugging subsystem. The function is called from hlt_done().
extern void __hlt_debug_done();

extern void __hlt_debug_print(const char* stream, const char* msg);

extern void __hlt_debug_printf_internal(const char* stream, const char* fmt, ...);

#endif
