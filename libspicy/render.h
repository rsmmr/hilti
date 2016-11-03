// Functions to render Spicy values into a readable ASCII presentation.

#ifndef LIBSPICY_RENDER_H
#define LIBSPICY_RENDER_H

#include <libhilti/rtti.h>

extern hlt_string spicy_object_to_string(const hlt_type_info* type, void* obj,
                                          hlt_exception** excpt, hlt_execution_context* ctx);

#endif
