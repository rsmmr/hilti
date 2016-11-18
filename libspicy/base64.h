
#ifndef LIBSPICY_BASE64_H

#include "libspicy.h"

extern hlt_bytes* spicy_base64_encode(hlt_bytes* b, hlt_exception** excpt,
                                      hlt_execution_context* ctx);
extern hlt_bytes* spicy_base64_decode(hlt_bytes* b, hlt_exception** excpt,
                                      hlt_execution_context* ctx);

#endif
