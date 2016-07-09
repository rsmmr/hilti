// Functions to render BinPAC++ values into a readable ASCII presentation.

#ifndef LIBBINPAC_RENDER_H
#define LIBBINPAC_RENDER_H

#include <libhilti/rtti.h>

extern hlt_string binpac_object_to_string(const hlt_type_info* type, void* obj,
                                          hlt_exception** excpt, hlt_execution_context* ctx);

#endif
