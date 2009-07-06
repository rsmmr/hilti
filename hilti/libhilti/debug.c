// $Id$

#include <string.h>

#include "hilti.h"
#include "list.h"

extern const __hlt_type_info __hlt_type_info_string;
extern const __hlt_type_info __hlt_type_info_tuple_string;

void __hlt_debug_printf(__hlt_string stream, __hlt_string fmt, const __hlt_type_info* type, const char* tuple, __hlt_exception* excpt)
{
    // FIXME: It's not very efficient to go through the stream-list every
    // time. Let's see if it's worth optimizing this even for the debug mode.
    // Once we have hash tables, this should be easy to do.
    const hilti_config* cfg = hilti_config_get();
    __hlt_list* streams = cfg->debug_streams;
    FILE* out = cfg->debug_out;
    
    if ( ! streams )
        return;
    
    __hlt_list_iter i = __hlt_list_begin(streams, excpt);
    __hlt_list_iter end = __hlt_list_end(streams, excpt);
    
    int found = 0;
    while ( ! __hlt_list_iter_eq(i, end, excpt) ) {
        __hlt_string* s = __hlt_list_iter_deref(i, excpt);
        if ( __hlt_string_cmp(stream, *s, excpt) == 0) {
            found = 1;
            break;
        }
        
        i = __hlt_list_iter_incr(i, excpt);
    }

    if ( *excpt )
        return;
    
    if ( ! found )
        // Not enabled.
        return;

    // FIXME: We should add a conveninece fmt() function taking (1) and const
    // char* fmt, and (2) varsargs rather than HILTI tuples.
    __hlt_string prefix = hilti_fmt(__hlt_string_from_asciiz("[%s] ", excpt), &__hlt_type_info_tuple_string, &stream, excpt);
    __hlt_string usr = hilti_fmt(fmt, type, tuple, excpt);
    
    if ( *excpt )
        return;
    
    __hlt_print_str(out, __hlt_string_concat(prefix, usr, excpt), 1, excpt);
    
    return;
}

__hlt_list* __hlt_debug_parse_streams(const char* streams, __hlt_exception* excpt)
{
    __hlt_list* l = __hlt_list_new(&__hlt_type_info_string, excpt);
    if ( *excpt )
        return 0;
    
    char* t;
    char* c;
    char *saveptr;
    
    char* copy = c = __hlt_gc_malloc_atomic(strlen(streams) + 1);
    strcpy(copy, streams);

    while ( (t = strtok_r(c, ":", &saveptr)) ) {
        __hlt_string s = __hlt_string_from_asciiz(t, excpt);
        __hlt_list_push_back(l, &__hlt_type_info_string, &s, excpt);
        if ( *excpt ) 
            return 0;
        
        c = 0;
    }
    
    return l;
}
    
