// $Id$

#include <string.h>

#include "hilti.h"
#include "list.h"

extern const hlt_type_info hlt_type_info_string;
extern const hlt_type_info hlt_type_info_tuple_string;

void hlt_debug_printf(hlt_string stream, hlt_string fmt, const hlt_type_info* type, const char* tuple, hlt_exception** excpt)
{
    // FIXME: It's not very efficient to go through the stream-list every
    // time. Let's see if it's worth optimizing this even for the debug mode.
    // Once we have hash tables, this should be easy to do.
    const hilti_config* cfg = hilti_config_get();
    hlt_list* streams = cfg->debug_streams;
    FILE* out = cfg->debug_out;
    
    if ( ! streams )
        return;
    
    hlt_list_iter i = hlt_list_begin(streams, excpt);
    hlt_list_iter end = hlt_list_end(streams, excpt);
    
    int found = 0;
    while ( ! hlt_list_iter_eq(i, end, excpt) ) {
        hlt_string* s = hlt_list_iter_deref(i, excpt);
        if ( hlt_string_cmp(stream, *s, excpt) == 0) {
            found = 1;
            break;
        }
        
        i = hlt_list_iter_incr(i, excpt);
    }

    if ( *excpt )
        return;
    
    if ( ! found )
        // Not enabled.
        return;

    // FIXME: We should add a conveninece fmt() function taking (1) and const
    // char* fmt, and (2) varsargs rather than HILTI tuples.
    hlt_string prefix = hilti_fmt(hlt_string_from_asciiz("[%s] ", excpt), &hlt_type_info_tuple_string, &stream, excpt);
    hlt_string usr = hilti_fmt(fmt, type, tuple, excpt);
    
    if ( *excpt )
        return;
    
    hlt_string_print(out, hlt_string_concat(prefix, usr, excpt), 1, excpt);
    
    return;
}

hlt_list* hlt_debug_parse_streams(const char* streams, hlt_exception** excpt)
{
    hlt_list* l = hlt_list_new(&hlt_type_info_string, excpt);
    if ( *excpt )
        return 0;
    
    char* t;
    char* c;
    char *saveptr;
    
    char* copy = c = hlt_gc_malloc_atomic(strlen(streams) + 1);
    strcpy(copy, streams);

    while ( (t = strtok_r(c, ":", &saveptr)) ) {
        hlt_string s = hlt_string_from_asciiz(t, excpt);
        hlt_list_push_back(l, &hlt_type_info_string, &s, excpt);
        if ( *excpt ) 
            return 0;
        
        c = 0;
    }
    
    return l;
}
    
