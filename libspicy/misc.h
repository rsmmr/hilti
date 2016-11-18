
#ifndef LIBSPICY_MISC_H

#include "libspicy.h"

hlt_string spicy_fmt_string(hlt_string fmt, const hlt_type_info* type, void* tuple,
                            hlt_exception** excpt, hlt_execution_context* ctx);
hlt_bytes* spicy_fmt_bytes(hlt_bytes* fmt, const hlt_type_info* type, void* tuple,
                           hlt_exception** excpt, hlt_execution_context* ctx);

#endif
