/* $Id$
 * 
 * Debugging support. 
 * 
 */

#ifndef DEBUG_H
#define DEBUG_H

void __hlt_debug_printf(__hlt_string stream, __hlt_string fmt, const __hlt_type_info* type, const char* tuple, __hlt_exception* excpt);

// Helper function for the host application. Converts a string of
// debug-stream names separated by colons into a list that can be stored into
// a "hilti_config".
__hlt_list* __hlt_debug_parse_streams(const char* streams, __hlt_exception* excpt);

#endif
