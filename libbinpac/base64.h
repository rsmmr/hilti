
#ifndef LIBBINPAC_BASE64_H

#include "libbinpac++.h"

extern hlt_bytes* binpac_base64_encode(hlt_bytes* b, hlt_exception** excpt,
                                       hlt_execution_context* ctx);
extern hlt_bytes* binpac_base64_decode(hlt_bytes* b, hlt_exception** excpt,
                                       hlt_execution_context* ctx);

#endif
