// $Id$
// 
// Library function available to the user as Hilti::*.

#ifndef HILTI_LIB_H
#define HILTI_LIB_H

#include "../hilti.h"

extern void hilti_print(const hlt_type_info* type, void* obj, int8_t newline, hlt_exception* excpt);
extern hlt_string hilti_fmt(hlt_string fmt, const hlt_type_info* type, const void* tuple, hlt_exception* excpt);

#endif
