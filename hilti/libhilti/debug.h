/* $Id$
 * 
 * Debugging support. 
 * 
 */

#ifndef HILTI_DEBUG_H
#define HILTI_DEBUG_H

#include "exceptions.h"
#include "list.h"

void hlt_debug_printf(hlt_string stream, hlt_string fmt, const hlt_type_info* type, const char* tuple, hlt_exception** excpt);

// Helper function for the host application. Converts a string of
// debug-stream names separated by colons into a list that can be stored into
// a "hilti_config".
hlt_list* hlt_debug_parse_streams(const char* streams, hlt_exception** excpt);

#endif
