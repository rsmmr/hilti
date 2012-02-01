//
// Library function available to the user as Hilti::*.
//

#ifndef HILTI_LIB_H
#define HILTI_LIB_H

#include "../hilti.h"

extern void hilti_print(const hlt_type_info* type, void* obj, int8_t newline, hlt_exception** excpt, hlt_execution_context* ctx);
extern hlt_string hilti_fmt(hlt_string fmt, const hlt_type_info* type, const void* tuple, hlt_exception** excpt, hlt_execution_context* ctx);

// extern void hilti_abort(hlt_exception** excpt);
// extern void hilti_sleep(double secs, hlt_exception** excpt, hlt_execution_context* ctx); // Doesn't yield!
// extern void hilti_wait_for_threads();

#endif
